#include "stdafx.h"
#include "TextureCompositor.h"

#include "GfxBase/AsyncResult.h"

#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/PartInstance.h"

#include "GfxBase/RenderCaps.h"
#include "GfxBase/RenderStats.h"

#include "VisualEngine.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "GlobalShaderData.h"

#include "GfxBase/FileMeshData.h"

#include "GfxCore/Geometry.h"
#include "GfxCore/Texture.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/Device.h"
#include "GfxCore/States.h"
#include "rbx/Profiler.h"

LOGVARIABLE(RenderTextureCompositor, 0)
LOGVARIABLE(RenderTextureCompositorBudget, 0)

namespace RBX
{
namespace Graphics
{

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
static const size_t kTextureCompositorActiveJobs = 2;
#else
static const size_t kTextureCompositorActiveJobs = 8;
#endif

static const size_t kTextureCompositorCooldown = 2;
static const double kRenderTextureCompositorRebakeDelaySeconds = 1.0;

static const unsigned int kTextureCompositorDefaultBudget = 8 * 1024 * 1024;
static const unsigned int kTextureCompositorOrphanedBudgetLimit = 32 * 1024 * 1024;
static const unsigned int kTextureCompositorOrphanedKeepAlive = 2;

#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
// OpenGL blits are really slow on iPad since they go through system memory (~240 ms for 1024x512)
static const bool kTextureCompositorUseRenderTextures = true;

// Conserve memory by using 16-bit textures
static const bool kTextureCompositorUse32BitTextures = false;
#else
static const bool kTextureCompositorUseRenderTextures = false;
static const bool kTextureCompositorUse32BitTextures = true;
#endif

static TextureCompositorConfiguration calculateConfiguration(const RenderCaps* caps)
{
	// Work around budget detection issues
	// An example of this is that when we run headless (RCCService), we'd like to have some reasonable default for budget size.
	// This also applies if the driver misreport VRAM size as zero.
	unsigned int budget =
		FLog::RenderTextureCompositorBudget
		? FLog::RenderTextureCompositorBudget * 1024 * 1024
		: std::max(kTextureCompositorDefaultBudget, static_cast<unsigned int>(caps->getVidMemSize() / 3));

	TextureCompositorConfiguration config;
	config.bpp = kTextureCompositorUse32BitTextures ? 32 : 16;
	config.budget = budget;
	
	return config;
}

static unsigned int getTextureSize(const shared_ptr<Texture>& tex)
{
	return tex ? Texture::getImageSize(tex->getFormat(), tex->getWidth(), tex->getHeight()) : 0;
}

static unsigned int getTextureSize(unsigned int width, unsigned int height, unsigned int bpp)
{
	return width * height * (bpp / 8);
}

template <typename T> struct WeakPtrEqualPredicate
{
    const boost::weak_ptr<T>* lhs;
    
    WeakPtrEqualPredicate(const boost::weak_ptr<T>* lhs): lhs(lhs)
    {
    }
    
    bool operator()(const boost::weak_ptr<T>& rhs) const
    {
        return !(*lhs < rhs || rhs < *lhs);
    }
};

template <typename T> struct ExistsInSetPredicate
{
    const std::set<T>* set;
    
    ExistsInSetPredicate(const std::set<T>* set): set(set)
    {
    }
    
    bool operator()(const T& object) const
    {
        return set->count(object) > 0;
    }
};

template <typename T> struct PriorityComparator
{
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs->priority < rhs->priority;
    }
};

struct DistanceUpdatePredicate
{
    void* job;
    Vector3 focus;
    float* distance;
    
    DistanceUpdatePredicate(void* job, const Vector3& focus, float* distance): job(job), focus(focus), distance(distance)
    {
    }
    
    bool operator()(const boost::weak_ptr<PartInstance>& locptr) const
    {
        boost::shared_ptr<PartInstance> loc = locptr.lock();
        if (!loc)
        {
            FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: detach instance", job);
            return true;
        }
        
        *distance = std::min(*distance, (focus - loc->getCoordinateFrame().translation).squaredMagnitude());
        return false;
    }
};

template <typename T> struct TextureIsNullPredicate
{
    bool operator()(const T& obj) const
    {
        return !obj->texture;
    }
};

std::string TextureCompositorLayer::toString() const
{
    return format("L[%s:%s:%08x:%d]", mesh.c_str(), texture.c_str(), Color4uint8(color).asUInt32(), mode);
}

class TextureCompositorMeshCache
{
public:
	TextureCompositorMeshCache(VisualEngine* visualEngine)
		: visualEngine(visualEngine)
	{
		std::vector<VertexLayout::Element> elements;
		elements.push_back(VertexLayout::Element(0, offsetof(Vertex, position), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
		elements.push_back(VertexLayout::Element(0, offsetof(Vertex, uv), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));

		layout = visualEngine->getDevice()->createVertexLayout(elements);
	}

    const shared_ptr<GeometryBatch>* requestMesh(const MeshId& meshId)
	{
        // Check mesh cache first; it's likely to have the mesh loaded
        Meshes::iterator it = meshes.find(meshId);

        if (it != meshes.end())
			return &it->second;

        // Try to fetch the mesh using content provider
        MeshContentProvider* mcp = visualEngine->getMeshContentProvider();
        
        AsyncHttpQueue::RequestResult result;
        shared_ptr<FileMeshData> cachedMesh = boost::static_pointer_cast<FileMeshData>(mcp->requestContent(meshId, ContentProvider::PRIORITY_MESH, false, result));

		if (result == AsyncHttpQueue::Waiting)
            return NULL;

        shared_ptr<GeometryBatch>& cache = meshes[meshId];

		if (result == AsyncHttpQueue::Succeeded)
		{
            // Fill cache with new mesh
            GeometryBatch mesh = createMesh(visualEngine->getDevice(), cachedMesh.get());

            cache.reset(new GeometryBatch(mesh));
		}

        return &cache;
    }

private:
    struct Vertex
    {
        Vector3 position;
        Vector2 uv;
    };

    VisualEngine* visualEngine;

    shared_ptr<VertexLayout> layout;

	typedef boost::unordered_map<MeshId, shared_ptr<GeometryBatch> > Meshes;
	Meshes meshes;

    GeometryBatch createMesh(Device* device, FileMeshData* data)
    {
        // Create and fill vertex buffer
		shared_ptr<VertexBuffer> vbuf = device->createVertexBuffer(sizeof(Vertex), data->vnts.size(), GeometryBuffer::Usage_Static);
        
        Vertex* vbptr = static_cast<Vertex*>(vbuf->lock());
        
        for (size_t i = 0; i < data->vnts.size(); ++i)
        {
            const FileMeshVertexNormalTexture3d& v = data->vnts[i];
            
            vbptr[i].position = Vector3(v.vx, v.vy, v.vz);
            vbptr[i].uv = Vector2(v.tu, v.tv);
        }
        
        vbuf->unlock();
        
        // Create and fill index buffer
		shared_ptr<IndexBuffer> ibuf = device->createIndexBuffer(sizeof(unsigned short), data->faces.size() * 3, GeometryBuffer::Usage_Static);

        unsigned short* ibptr = static_cast<unsigned short*>(ibuf->lock());
        
        for (size_t i = 0; i < data->faces.size(); ++i)
        {
            const FileMeshFace& f = data->faces[i];
            
            ibptr[i * 3 + 0] = f.a;
            ibptr[i * 3 + 1] = f.b;
            ibptr[i * 3 + 2] = f.c;
        }
        
        ibuf->unlock();
        
        // Create geometry batch
		shared_ptr<Geometry> geometry = device->createGeometry(layout, vbuf, ibuf);

		return GeometryBatch(geometry, Geometry::Primitive_Triangles, data->faces.size() * 3, data->vnts.size());
    }
};

class TextureCompositorJob
{
public:
    TextureCompositorJob(VisualEngine* visualEngine, const Vector2& canvasSize, const std::vector<TextureCompositorLayer>& layers, float contentPriority, const std::string& context)
        : visualEngine(visualEngine)
        , canvasSize(canvasSize)
        , layers(layers.size())
        , contentPriority(contentPriority)
        , readyCount(0)
    {
        for (size_t i = 0; i < layers.size(); ++i)
        {
            LayerData& ld = this->layers[i];

            ld.desc = layers[i];

            // Fetch texture
			if (!ld.desc.texture.isNull())
			{
				ld.texture = visualEngine->getTextureManager()->load(ld.desc.texture, TextureManager::Fallback_None, context);
			}
			else
			{
				ld.texture = visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_White);
			}
        }
    }
    
    void update(TextureCompositorMeshCache& meshCache)
    {
        size_t ready = 0;
        
        for (size_t i = 0; i < layers.size(); ++i)
        {
            LayerData& ld = layers[i];
            
            // Fetch mesh
			if (!ld.mesh)
				ld.mesh = meshCache.requestMesh(ld.desc.mesh);

            // Verify mesh/texture
			if (ld.mesh && ld.texture.getStatus() != TextureRef::Status_Waiting)
                ready++;
        }
        
        readyCount = ready;
	}

	void render(DeviceContext* context, const shared_ptr<Framebuffer>& framebuffer)
    {
        RBXASSERT(isReady());

		const DeviceCaps& caps = visualEngine->getDevice()->getCaps();

        // half-pixel offset for D3D to account for shifted pixel center during rasterization
		float offset = caps.needsHalfPixelOffset ? 0.5f : 0;

        Matrix4 projection =
            Matrix4(
                2.f / canvasSize.x, 0.f, 0.f, -1.f - offset * 2.f / static_cast<float>(framebuffer->getWidth()),
                0.f, 2.f / canvasSize.y, 0.f, -1.f + offset * 2.f / static_cast<float>(framebuffer->getHeight()),
                0.f, 0.f, 0.001f, 0.5f, // shift depth to 0.5 to avoid clipping; don't multiply Z by zero to avoid d3d debug warnings.
                0.f, 0.f, 0.f, 1.f);

		if (caps.requiresRenderTargetFlipping)
		{
            projection.setRow(1, -projection.row(1));
		}

        RenderCamera orthoCamera;
        orthoCamera.setViewMatrix(Matrix4::identity());
		orthoCamera.setProjectionMatrix(projection);

		GlobalShaderData globalData;
		globalData.setCamera(orthoCamera);

		context->updateGlobalConstants(&globalData, sizeof(globalData));

		context->setRasterizerState(caps.requiresRenderTargetFlipping ? RasterizerState::Cull_Front : RasterizerState::Cull_Back);
		context->setDepthState(DepthState(DepthState::Function_Always, false));
        
		context->bindFramebuffer(framebuffer.get());

		const float clearColor[] = {0.5f, 0.5f, 0.5f, 0.f};
		context->clearFramebuffer(DeviceContext::Buffer_Color, clearColor, 1.f, 0);
        
        for (size_t i = 0; i < layers.size(); ++i)
        {
            LayerData& ld = layers[i];

			shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP("TexCompVS", ld.desc.mode == TextureCompositorLayer::Composit_BlendTexture ? "TexCompPMAFS" : "TexCompFS");

			if (program && *ld.mesh && ld.texture.getTexture())
			{
				context->setBlendState(ld.desc.mode == TextureCompositorLayer::Composit_BlendTexture ? BlendState::Mode_PremultipliedAlphaBlend : BlendState::Mode_None);

				context->bindProgram(program.get());

				context->bindTexture(0, ld.texture.getTexture().get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

                float colorData[] = {ld.desc.color.r, ld.desc.color.g, ld.desc.color.b, (ld.desc.mode == TextureCompositorLayer::Composit_BlitTextureAlphaMagnify4x) ? 4.f : 1.f};
                context->setConstant(program->getConstantHandle("Color"), colorData, 1);

				context->draw(**ld.mesh);
			}
        }
    }
   
    bool isReady() const
	{
        return readyCount == layers.size();
	}

private:
    struct LayerData
    {
        LayerData(): desc(MeshId(), TextureId()), mesh(NULL)
        {
        }
        
        TextureCompositorLayer desc;

        const shared_ptr<GeometryBatch>* mesh;
        TextureRef texture;
    };
    
    VisualEngine* visualEngine;
    
    Vector2 canvasSize;
    
    std::vector<LayerData> layers;
    float contentPriority;
    
    size_t readyCount;
};

TextureCompositor::RenderedJob::RenderedJob()
	: cooldown(0)
{
}

TextureCompositor::RenderedJob::RenderedJob(const shared_ptr<Job>& job, const shared_ptr<Framebuffer>& framebuffer, int cooldown)
	: job(job)
    , framebuffer(framebuffer)
    , cooldown(cooldown)
{
}

TextureCompositor::TextureCompositor(VisualEngine* visualEngine)
	: Resource(visualEngine->getDevice())
    , visualEngine(visualEngine)
    , lastActiveTime(Time::now<Time::Fast>())
{
    // get configuration based on available VRAM
    config = calculateConfiguration(visualEngine->getRenderCaps());

	meshCache.reset(new TextureCompositorMeshCache(visualEngine));
}

TextureCompositor::~TextureCompositor()
{
}

TextureCompositor::JobHandle TextureCompositor::getJob(const std::string& textureid, const std::string& context, unsigned int width, unsigned int height, const Vector2& canvasSize, const std::vector<TextureCompositorLayer>& layers)
{
    // look for an existing job
    boost::shared_ptr<Job>& job = jobs[textureid];
    if (job) return job;
    
    // look for an orphaned job with the same id
    for (size_t i = 0; i < orphanedJobs.size(); ++i)
    {
        const boost::shared_ptr<Job>& orphaned = orphanedJobs[i];
        
        if (orphaned->desc.textureid == textureid && orphaned->texture)
        {
            // we recently orphaned a job with the same description and a texture which is still valid
            // we have to readd the job to job list, but we don't have to rebake it - just reuse the contents
            FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: restoring orphaned job", orphaned.get());
            
            job = orphaned;
			job->textureRef = job->texture;

            orphanedJobs.erase(orphanedJobs.begin() + i);
            
            return job;
        }
    }
    
    // create new job
    job.reset(new Job());
    
    job->desc.textureid = textureid;
	job->desc.width = width;
	job->desc.height = height;
    job->desc.canvasSize = canvasSize;
    job->desc.layers = layers;
	job->context = context;
    
    job->priority = FLT_MAX;

	job->textureRef = TextureRef::future(visualEngine->getTextureManager()->getFallbackTexture(TextureManager::Fallback_Gray));
    
    FASTLOG4(FLog::RenderTextureCompositor, "TC Job[%p]: create %dx%d (%d layers)", job.get(), width, height, layers.size());
    FASTLOGS(FLog::RenderTextureCompositor, "TC Job texture: %s", textureid.c_str());
    
    for (size_t i = 0; i < layers.size(); ++i)
    {
        FASTLOGS(FLog::RenderTextureCompositor, "TC Job layer: %s", layers[i].toString().c_str());
    }
    
    // queue job for processing on subsequent frames
    pendingJobs.push_back(job);
    
    FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: queueing (external request)", job.get());
    
    return job;
}

TextureRef TextureCompositor::getTexture(const JobHandle& job)
{
    RBXASSERT(job);
    
    return job->textureRef;
}

const std::string& TextureCompositor::getTextureId(const JobHandle& job)
{
    RBXASSERT(job);
    
    return job->desc.textureid;
}

void TextureCompositor::attachInstance(const JobHandle& job, const boost::shared_ptr<PartInstance>& instance)
{
    RBXASSERT(job);
    
    if (!instance) return;
    
    boost::weak_ptr<PartInstance> weakptr = instance;
    
    if (std::find_if(job->instances.begin(), job->instances.end(), WeakPtrEqualPredicate<PartInstance>(&weakptr)) == job->instances.end())
    {
        job->instances.push_back(weakptr);
        
        FASTLOG2(FLog::RenderTextureCompositor, "TC Job[%p]: attach instance %p", job.get(), instance.get());
        FASTLOGS(FLog::RenderTextureCompositor, "TC Job instance: %s", instance->getFullName().c_str());
    }
}

bool TextureCompositor::isQueueEmpty() const
{
	return pendingJobs.empty() && activeJobs.empty() && !renderedJob.job;
}

void TextureCompositor::updatePrioritiesAndOrphanJobs(const Vector3& pointOfInterest)
{
    std::set<boost::shared_ptr<Job> > newOrphanedJobs;
    
    // update priorities for all jobs and keep track of jobs we no longer need
    for (JobMap::iterator it = jobs.begin(); it != jobs.end(); )
    {
        Job& job = *it->second;
        
        // update priority and discard dead instances
        float distance = FLT_MAX;
        
        job.instances.erase(std::remove_if(job.instances.begin(), job.instances.end(), DistanceUpdatePredicate(&job, pointOfInterest, &distance)), job.instances.end());
        
        job.priority = distance;
        
        // remove jobs where texture is not needed any more (no point regenerating the texture)
		if (job.textureRef.isUnique())
        {
            FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: orphaning job (no materials)", &job);
            
            // cleanup job state
			job.textureRef = TextureRef();
            job.job.reset();
            job.instances.clear();
            
			newOrphanedJobs.insert(it->second);
            
            // erase old job from map, move iterator to the next one
            jobs.erase(it++);
        }
        else
            ++it;
    }
    
    if (!newOrphanedJobs.empty())
    {
        // add all orphaned jobs to orphaned queue (we'll garbage collect them separately)
        orphanedJobs.insert(orphanedJobs.end(), newOrphanedJobs.begin(), newOrphanedJobs.end());
    
        // remove orphaned jobs from all queues
		ExistsInSetPredicate<boost::shared_ptr<Job> > isOrphaned(&newOrphanedJobs);
        
        pendingJobs.erase(std::remove_if(pendingJobs.begin(), pendingJobs.end(), isOrphaned), pendingJobs.end());
        activeJobs.erase(std::remove_if(activeJobs.begin(), activeJobs.end(), isOrphaned), activeJobs.end());
    }
}

void TextureCompositor::garbageCollectOrphanedJobs()
{
    unsigned int totalSize = getTotalLiveTextureSize();
    unsigned int orphanedSize = getTotalOrphanedTextureSize();
    
    // we always keep N low-res textures so that downsampling or creating a new outfit does not require a texture allocation
    // otherwise we cap the total orphaned size by remaining budget, but no more than 25% of the budget and no more than a fixed limit
    unsigned int maxOrphanedSize = (totalSize > config.budget) ? 0 : std::min(config.budget - totalSize, std::min(config.budget / 4, kTextureCompositorOrphanedBudgetLimit));
    
    if (orphanedSize > maxOrphanedSize)
    {
        std::vector<char> sweep(orphanedJobs.size());
        
        // sweep textures starting from the front (textures at the back are LRU) while we exceed the orphaned size budget
        for (size_t i = 0; i < orphanedJobs.size(); ++i)
        {
            if (orphanedSize > maxOrphanedSize)
            {
                orphanedSize -= getTextureSize(orphanedJobs[i]->texture);
                sweep[i] = true;
            }
        }
        
        // don't sweep last N low-res textures
        unsigned int keepAlive = kTextureCompositorOrphanedKeepAlive;
        
        for (size_t i = orphanedJobs.size(); i > 0; --i)
        {
			const shared_ptr<Job>& job = orphanedJobs[i - 1];
            const shared_ptr<Texture>& texture = job->texture;
            
			if (texture && texture->getWidth() < job->desc.width && keepAlive > 0)
            {
                keepAlive--;
                sweep[i - 1] = false;
            }
        }
        
        // sweep!
        for (size_t i = 0; i < sweep.size(); ++i)
        {
            shared_ptr<Texture>& texture = orphanedJobs[i]->texture;
            
            if (sweep[i] && texture)
            {
                FASTLOG1(FLog::RenderTextureCompositor, "TC Destroy texture %p", texture.get());
                
                texture.reset();
                
				if (orphanedJobs[i] == renderedJob.job)
                {
                    FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: cancelling blit in progress since the texture is destroyed", orphanedJobs[i].get());
                    
                    renderedJob = RenderedJob();
                }
            }
        }
    }
    
    // destroy all orphaned jobs that did not have a texture or don't have a texture after cleanup above
    orphanedJobs.erase(std::remove_if(orphanedJobs.begin(), orphanedJobs.end(), TextureIsNullPredicate<boost::shared_ptr<Job> >()), orphanedJobs.end());
}

void TextureCompositor::findRebakeTargetAndEnqueue()
{
    unsigned int totalSize = getTotalLiveTextureSize();
    
    // gather all jobs and sort by priority
    std::vector<boost::shared_ptr<Job> > sortedJobs;
    sortedJobs.reserve(jobs.size());
    
    for (JobMap::const_iterator it = jobs.begin(); it != jobs.end(); ++it)
        sortedJobs.push_back(it->second);
        
    std::sort(sortedJobs.begin(), sortedJobs.end(), PriorityComparator<boost::shared_ptr<Job> >());
    
    // get indices for left-most low-res and right-most high-res texture
    size_t leftMostLow = sortedJobs.size();
    size_t rightMostHighPlus1 = 0;
    
    for (size_t i = 0; i < sortedJobs.size(); ++i)
    {
		const shared_ptr<Job>& job = sortedJobs[i];
        const shared_ptr<Texture>& texture = job->texture;
        
        if (texture)
        {
			if (texture->getWidth() < job->desc.width)
                leftMostLow = std::min(leftMostLow, i);
            else
                rightMostHighPlus1 = std::max(rightMostHighPlus1, i + 1);
        }
    }
    
        // we can upsample something
        if (leftMostLow < sortedJobs.size())
        {
		const shared_ptr<Job>& job = sortedJobs[leftMostLow];

		if (totalSize + getTextureSize(job->desc.width, job->desc.height, config.bpp) < config.budget)
		{
			FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: queueing (upsample)", job.get());
            
			pendingJobs.push_back(job);
			return;
        }
    }

        // we can downsample something
        if (rightMostHighPlus1 > 0)
        {
            size_t rightMostHigh = rightMostHighPlus1 - 1;
            
            // equilibrium conditions:
            // - all high-res textures have priority less than all low-res textures
            // - we are below texture budget
            if (rightMostHigh < leftMostLow && totalSize < config.budget)
                ;
            else
            {
                FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: queueing (downsample)", sortedJobs[rightMostHigh].get());
            
                pendingJobs.push_back(sortedJobs[rightMostHigh]);
            }
        }
    }

void TextureCompositor::garbageCollectFull()
{
	updatePrioritiesAndOrphanJobs(Vector3());

    for (size_t i = 0; i < orphanedJobs.size(); ++i)
    {
        shared_ptr<Texture>& texture = orphanedJobs[i]->texture;
            
        if (texture)
        {
            FASTLOG1(FLog::RenderTextureCompositor, "TC Destroy texture %p", texture.get());
            
            texture.reset();
            
            if (orphanedJobs[i] == renderedJob.job)
            {
                FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: cancelling blit in progress since the texture is destroyed", orphanedJobs[i].get());
                
                renderedJob = RenderedJob();
            }
        }
    }

    orphanedJobs.clear();
}

void TextureCompositor::cancelPendingRequests()
{
	jobs.clear();

	pendingJobs.clear();
	activeJobs.clear();
	orphanedJobs.clear();

	renderedJob = RenderedJob();
}

void TextureCompositor::update(const Vector3& pointOfInterest)
{
    // this gets rid of all jobs that we don't need to process anyway
    updatePrioritiesAndOrphanJobs(pointOfInterest);
    
    // orphaned queue serves as a cache and a texture pool at the same time; make sure it does not grow too big
    garbageCollectOrphanedJobs();
    
    // if we have nothing else to do, let's try to find some textures that need to have a different resolution
    if (isQueueEmpty())
    {
        double latency = (Time::now<Time::Fast>() - lastActiveTime).seconds();
        
        if (latency >= kRenderTextureCompositorRebakeDelaySeconds)
        {
            findRebakeTargetAndEnqueue();
        }
    }
    else
    {
        lastActiveTime = Time::now<Time::Fast>();
    }
    
    // move some jobs from pending to active
    if (pendingJobs.size() > 0 && activeJobs.size() < kTextureCompositorActiveJobs)
    {
        size_t count = std::min(pendingJobs.size(), kTextureCompositorActiveJobs - activeJobs.size());
        
        std::nth_element(pendingJobs.begin(), pendingJobs.begin() + count, pendingJobs.end(), PriorityComparator<boost::shared_ptr<Job> >());
        
        activeJobs.insert(activeJobs.end(), pendingJobs.begin(), pendingJobs.begin() + count);
        pendingJobs.erase(pendingJobs.begin(), pendingJobs.begin() + count);
    }
    
    // update active job order (important for renderJobIfNecessary)
    std::sort(activeJobs.begin(), activeJobs.end(), PriorityComparator<boost::shared_ptr<Job> >());
        
    // update active jobs
    for (size_t i = 0; i < activeJobs.size(); ++i)
    {
        updateJob(*activeJobs[i]);
    }
}

void TextureCompositor::render(DeviceContext* context)
{
    RBXPROFILER_SCOPE("Render", "TextureCompositor::render");
    RBXPROFILER_SCOPE("GPU", "TextureCompositor::render");
	if (renderedJob.job)
    {
        // finalize the job that we rendered before
		--renderedJob.cooldown;
        
		if (renderedJob.cooldown <= 0)
        {
			renderJobFinalize(*renderedJob.job, renderedJob.framebuffer, context);
			renderedJob = RenderedJob();
        }
    }
    else
    {
        // render at most one active job
        for (size_t i = 0; i < activeJobs.size(); ++i)
        {
            Job& job = *activeJobs[i];
            
            if (job.job && job.job->isReady())
            {
                // render and update materials to use new texture
                renderJobIfNecessary(job, i, context);
                
                // make sure we don't keep render data alive
                job.job.reset();
                
                // remove job from active queue
                activeJobs.erase(activeJobs.begin() + i);
                break;
            }
        }
    }
}

void TextureCompositor::updateJob(Job& job)
{
    if (!job.job)
    {
        FASTLOG2(FLog::RenderTextureCompositor, "TC Job[%p]: start loading assets (priority %d)", &job, (int)sqrtf(job.priority));

        // convert from [0..+inf) to [0..1] while keeping the ordering; distribution is not very important
        float priority = 1.f - 1.f / (1.f + sqrtf(job.priority));

        job.job.reset(new TextureCompositorJob(visualEngine, job.desc.canvasSize, job.desc.layers, ContentProvider::PRIORITY_CHARACTER + priority, job.context));
    }

	job.job->update(*meshCache);
}

void TextureCompositor::renderJobFinalize(Job& job, const shared_ptr<Framebuffer>& framebuffer, DeviceContext* context)
{
    const shared_ptr<Texture>& texture = job.texture;
    RBXASSERT(texture);
    
	if (texture->getUsage() != Texture::Usage_Renderbuffer)
    {
        try
		{
            Timer<Time::Precise> timer;

            context->copyFramebuffer(framebuffer.get(), texture.get());
            
            FASTLOG3(FLog::RenderTextureCompositor, "TC Job[%p]: texture blit (width %d) took %d us", &job, texture->getWidth(), (int)(timer.delta().msec() * 1000));
		}
        catch (const RBX::base_exception& e)
		{
            FASTLOG2(FLog::RenderTextureCompositor, "TC Job[%p]: texture blit (width %d) failed", &job, texture->getWidth());
            FASTLOGS(FLog::RenderTextureCompositor, "TC: Failure reason %s", e.what());

            RBX::StandardOut::singleton()->printf(MESSAGE_OUTPUT,"TextureCompositor copyFramebuffer failed: %s", e.what());
		}
    }

    // update texture reference
    if (job.textureRef.getStatus() != TextureRef::Status_Null)
        job.textureRef.updateAllRefs(texture);
}

void TextureCompositor::renderJobIfNecessary(Job& job, size_t activePosition, DeviceContext* context)
{
    unsigned int totalSize = getTotalLiveTextureSize();
    
    // assuming that all other textures in the queue are new (they should be, since we don't rebake with non-empty queue),
    // we can estimate a final texture size better if we assume that we already created all other textures with suitable quality
    unsigned int pendingSize =
		getProjectedPendingTextureSize() / 4 + // all pending jobs are assumed to get low-res texture
		getProjectedActiveTextureSize(activePosition + 1, activeJobs.size()) / 4 + // all active jobs after us are assumed to get low-res texture
		getProjectedActiveTextureSize(0, activePosition); // all active jobs before us are assumed to get high-res textures (even if our assets load first we don't get high-res texture out of order)
    
    // create a high-quality texture only if we're within budget after we create the texture
	unsigned int textureSize = getTextureSize(job.desc.width, job.desc.height, config.bpp);

	bool hq = (totalSize + pendingSize + textureSize < config.budget);

	unsigned int width = hq ? job.desc.width : job.desc.width / 2;
	unsigned int height = hq ? job.desc.height : job.desc.height / 2;
    
	if (!job.texture || job.texture->getWidth() != width || job.texture->getHeight() != height)
    {
        if (job.texture)
        {
            // We're resampling an existing job; rather than lose the texture, let's add it to orphaned queue as an empty job - garbage collection will take care of it
            orphanTextureFromJob(job);
        }
        
        FASTLOG5(FLog::RenderTextureCompositor, "TC Job[%p]: render (totalSize %d, pendingSize %d, budget %d -> width %d)", &job, totalSize, pendingSize, config.budget, width);
        
        shared_ptr<Texture> texture = getOrCreateTexture(width, height);
		shared_ptr<Framebuffer> framebuffer = getOrCreateFramebufer(texture);

        job.job->render(context, framebuffer);

        // make sure the job uses the texture so that we can finalize it later at some point
        job.texture = texture;
    
        // queue job for some final processing
        RBXASSERT(activeJobs[activePosition].get() == &job);
		renderedJob = RenderedJob(activeJobs[activePosition], framebuffer, kTextureCompositorCooldown);
    }
}

void TextureCompositor::orphanTextureFromJob(Job& job)
{
    boost::shared_ptr<Job> textureJob(new Job());
    
    textureJob->priority = FLT_MAX;
    textureJob->texture = job.texture;
    
    FASTLOG2(FLog::RenderTextureCompositor, "TC Job[%p]: store previous texture as orphaned job %p", &job, textureJob.get());
    
    orphanedJobs.push_back(textureJob);
    
	job.texture.reset();
}

unsigned int TextureCompositor::getTotalLiveTextureSize()
{
    unsigned int result = 0;
    
    for (JobMap::const_iterator it = jobs.begin(); it != jobs.end(); ++it)
        result += getTextureSize(it->second->texture);
    
    return result;
}

unsigned int TextureCompositor::getTotalOrphanedTextureSize()
{
    unsigned int result = 0;
    
    for (size_t i = 0; i < orphanedJobs.size(); ++i)
        result += getTextureSize(orphanedJobs[i]->texture);
    
    return result;
}

unsigned int TextureCompositor::getProjectedPendingTextureSize()
{
    unsigned int result = 0;
    
    for (size_t i = 0; i < pendingJobs.size(); ++i)
	{
		const shared_ptr<Job>& job = pendingJobs[i];

		result += getTextureSize(job->desc.width, job->desc.height, config.bpp);
	}
    
    return result;
}

unsigned int TextureCompositor::getProjectedActiveTextureSize(size_t begin, size_t end)
{
    unsigned int result = 0;
    
    for (size_t i = begin; i < end; ++i)
{
		const shared_ptr<Job>& job = activeJobs[i];

		result += getTextureSize(job->desc.width, job->desc.height, config.bpp);
	}
    
    return result;
}

shared_ptr<Framebuffer> TextureCompositor::getOrCreateFramebufer(const shared_ptr<Texture>& texture)
{
    // we can render into the texture if it's a render target itself
	if (texture->getUsage() == Texture::Usage_Renderbuffer)
		return visualEngine->getDevice()->createFramebuffer(texture->getRenderbuffer(0, 0));
        
    // look for matching framebuffer in cache
	for (size_t i = 0; i < framebuffers.size(); ++i)
    {
		if (framebuffers[i]->getWidth() == texture->getWidth() && framebuffers[i]->getHeight() == texture->getHeight())
            return framebuffers[i];
    }
            
    // create a new framebuffer
	shared_ptr<Texture> rt = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_RGBA8, texture->getWidth(), texture->getHeight(), 1, 1, Texture::Usage_Renderbuffer);
	shared_ptr<Framebuffer> framebuffer = visualEngine->getDevice()->createFramebuffer(rt->getRenderbuffer(0, 0));
    
    // we make sure RTs are always alive to minimize stalls (there should be <3 Mb of them anyway)
    framebuffers.push_back(framebuffer);
    
    return framebuffer;
}

shared_ptr<Texture> TextureCompositor::getOrCreateTexture(unsigned int width, unsigned int height)
{
    // try to steal a texture from orphaned queue
    for (size_t i = 0; i < orphanedJobs.size(); ++i)
    {
        shared_ptr<Texture> texture = orphanedJobs[i]->texture;
        
		if (texture && texture->getWidth() == width && texture->getHeight() == height)
        {
            FASTLOG1(FLog::RenderTextureCompositor, "TC Reuse texture %p", texture.get());
        
            orphanedJobs.erase(orphanedJobs.begin() + i);
            
            return texture;
        }
    }
    
	Texture::Format format = config.bpp == 16 ? Texture::Format_RGB5A1 : Texture::Format_RGBA8;
	Texture::Usage usage = kTextureCompositorUseRenderTextures ? Texture::Usage_Renderbuffer : Texture::Usage_Static;
        
	shared_ptr<Texture> texture = visualEngine->getDevice()->createTexture(Texture::Type_2D, format, width, height, 1, 1, usage);

    FASTLOG1(FLog::RenderTextureCompositor, "TC Create texture %p", texture.get());
    
    return texture;
}

TextureCompositorStats TextureCompositor::getStatistics() const
{
    TextureCompositorStats result = {};
    
    for (JobMap::const_iterator it = jobs.begin(); it != jobs.end(); ++it)
    {
		const shared_ptr<Job>& job = it->second;
        const shared_ptr<Texture>& texture = job->texture;
        
        if (!texture)
            continue;
            
		if (texture->getWidth() == job->desc.width)
        {
            result.liveHQCount++;
            result.liveHQSize += getTextureSize(texture);
        }
        else
        {
            result.liveLQCount++;
            result.liveLQSize += getTextureSize(texture);
        }
    }
    
    for (size_t i = 0; i < orphanedJobs.size(); ++i)
    {
        result.orphanedCount++;
        result.orphanedSize += getTextureSize(orphanedJobs[i]->texture);
    }
    
    return result;
}

void TextureCompositor::onDeviceLost()
{
    FASTLOG(FLog::RenderTextureCompositor, "TC Device lost");
    
	if (shared_ptr<Job> job = renderedJob.job)
    {
        // We're going to blit the texture at some point in the future; however, we've just lost the texture contents.
        // Let's put the job back to the pending queue.
        FASTLOG1(FLog::RenderTextureCompositor, "TC Job[%p]: device lost while job render is in progress, enqueue job once again", job.get());
        
        orphanTextureFromJob(*job);
        pendingJobs.push_back(job);
        
		renderedJob = RenderedJob();
    }
}

}
}
