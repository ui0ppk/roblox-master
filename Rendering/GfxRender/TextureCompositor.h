#pragma once

#include "Util/MeshId.h"
#include "Util/TextureId.h"

#include "G3D/Vector2.h"
#include "G3D/Color4.h"

#include "TextureRef.h"

#include "GfxCore/Resource.h"

namespace RBX
{
	class AsyncResult;
	class MeshId;
	class ContentId;
	class ContentProvider;
	class PartInstance;
}

namespace RBX
{
namespace Graphics
{

class VisualEngine;
class DeviceContext;
class Framebuffer;

struct TextureCompositorLayer
{
    enum CompositMode
    {
        Composit_BlendTexture,
        Composit_BlitColor,
        Composit_BlitTextureAlphaMagnify4x
    };
    
    TextureCompositorLayer(const MeshId& mesh, const TextureId& texture, CompositMode mode = Composit_BlendTexture)
        : mesh(mesh)
        , texture(texture)
        , color(1.0f, 1.0f, 1.0f)
        , mode(mode)
    {
    }
    
    TextureCompositorLayer(const MeshId& mesh, const Color3& color)
        : mesh(mesh)
        , color(color)
        , mode(Composit_BlitColor)
    {
    }

    MeshId mesh;
    TextureId texture;
    Color3 color;
    CompositMode mode;

    std::string toString() const;
};

struct TextureCompositorConfiguration
{
    unsigned int bpp;
    unsigned int budget;
};

struct TextureCompositorStats
{
    unsigned int liveHQCount;
    unsigned int liveHQSize;
    unsigned int liveLQCount;
    unsigned int liveLQSize;
    unsigned int orphanedCount;
    unsigned int orphanedSize;
};

class TextureCompositorJob;
class TextureCompositorMeshCache;

class TextureCompositor: public Resource
{
    struct Job;
    
public:
    TextureCompositor(VisualEngine* visualEngine);
    ~TextureCompositor();
    
    typedef shared_ptr<Job> JobHandle;
    
    JobHandle getJob(const std::string& textureid, const std::string& context, unsigned int width, unsigned int height, const Vector2& canvasSize, const std::vector<TextureCompositorLayer>& layers);
    
    TextureRef getTexture(const JobHandle& job);
    const std::string& getTextureId(const JobHandle& job);
    
    void attachInstance(const JobHandle& job, const shared_ptr<PartInstance>& instance);

    void garbageCollectFull();

	void cancelPendingRequests();

    void update(const Vector3& pointOfInterest);
    void render(DeviceContext* context);

    bool isQueueEmpty() const;
    
    const TextureCompositorConfiguration& getConfiguration() const { return config; }
    TextureCompositorStats getStatistics() const;

    virtual void onDeviceLost();

private:
    struct JobDescriptor
    {
        std::string textureid;
        
		unsigned int width, height;
        Vector2 canvasSize;
        std::vector<TextureCompositorLayer> layers;
    };
    
    struct Job
    {
        JobDescriptor desc;
        
        std::vector<weak_ptr<PartInstance> > instances;
        
        float priority;
		std::string context;
        
        shared_ptr<Texture> texture;
        TextureRef textureRef;

        shared_ptr<TextureCompositorJob> job;
    };

    struct RenderedJob
	{
        shared_ptr<Job> job;
        shared_ptr<Framebuffer> framebuffer;
        int cooldown;

        RenderedJob();
        RenderedJob(const shared_ptr<Job>& job, const shared_ptr<Framebuffer>& framebuffer, int cooldown);
	};
    
    VisualEngine* visualEngine;
    TextureCompositorConfiguration config;

    scoped_ptr<TextureCompositorMeshCache> meshCache;
    
    typedef std::map<std::string, shared_ptr<Job> > JobMap;
    JobMap jobs;
    
    std::vector<shared_ptr<Job> > pendingJobs;
    std::vector<shared_ptr<Job> > activeJobs;
    std::vector<shared_ptr<Job> > orphanedJobs;
    
    RenderedJob renderedJob;
    
    std::vector<shared_ptr<Framebuffer> > framebuffers;
    
    Time lastActiveTime;
    
    void updatePrioritiesAndOrphanJobs(const Vector3& pointOfInterest);
    void garbageCollectOrphanedJobs();
    void findRebakeTargetAndEnqueue();
    
    void updateJob(Job& job);
    void renderJobIfNecessary(Job& job, size_t activePosition, DeviceContext* context);
    void renderJobFinalize(Job& job, const shared_ptr<Framebuffer>& framebuffer, DeviceContext* context);
    
    void orphanTextureFromJob(Job& job);
    
    unsigned int getTotalLiveTextureSize();
    unsigned int getTotalOrphanedTextureSize();

    unsigned int getProjectedPendingTextureSize();
    unsigned int getProjectedActiveTextureSize(size_t begin, size_t end);

    shared_ptr<Framebuffer> getOrCreateFramebufer(const shared_ptr<Texture>& texture);
    shared_ptr<Texture> getOrCreateTexture(unsigned int width, unsigned int height);
};

}
}
