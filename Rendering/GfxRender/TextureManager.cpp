#include "stdafx.h"
#include "TextureManager.h"

#include "Image.h"
#include "VisualEngine.h"

#include "v8datamodel/ContentProvider.h"

#include "GfxBase/RenderCaps.h"
#include "GfxCore/Device.h"

#include "util/ThreadPool.h"
#include "util/Statistics.h"
#include "StringConv.h"
#include "rbx/Profiler.h"

LOGGROUP(Graphics)

FASTINTVARIABLE(RenderTextureManagerBudget, 0)
FASTINTVARIABLE(RenderTextureManagerBudgetFor4k, 0)
DYNAMIC_FASTFLAG(ImageFailedToLoadContext)

namespace RBX
{
namespace Graphics
{

static const unsigned int kTextureManagerThreads = 1;
static const unsigned int kTextureManagerRequestsPerFrame = 1;

static const unsigned int kTextureManagerMinBudget = 32 * 1024 * 1024;
static const unsigned int kTextureManagerOrphanedBudgetLimit = 64 * 1024 * 1024;

static void logError(const ContentId& id, const std::string& context, const char* error)
{
    FASTLOGS(FLog::Graphics, "Image failed to load: %s", id.toString() + ": " + error);

	if (DFFlag::ImageFailedToLoadContext && context != "") 
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Image failed to load %s : %s because %s", context.c_str(), id.c_str(), error);
	}
	else 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Image failed to load: %s: %s", id.c_str(), error);
	}
}

static void httpCallback(AsyncHttpQueue::RequestResult result, std::istream* stream, const shared_ptr<const std::string>& content, boost::function<void (shared_ptr<const std::string>)> callback)
{
	if (result == AsyncHttpQueue::Succeeded)
        callback(content);
	else
        callback(shared_ptr<const std::string>());
}

static unsigned int getTextureSize(const shared_ptr<Texture>& tex)
{
    unsigned int result = 0;

	for (unsigned int mip = 0; mip < tex->getMipLevels(); ++mip)
		result += Texture::getImageSize(tex->getFormat(), Texture::getMipSide(tex->getWidth(), mip), Texture::getMipSide(tex->getHeight(), mip)) * Texture::getMipSide(tex->getDepth(), mip);

	return result * (tex->getType() == Texture::Type_Cube ? 6 : 1);
}

static unsigned int getMaxTextureSize(const DeviceCaps& caps, unsigned int totalBudget, bool isLocal)
{
	// Limit texture size by HW restrictions for local assets and by available VRAM for non-local assets
    // If we don't support NPOT, limit texture size by 512 to restrict upscaling
	if (isLocal)
	{
		// Even if your hardware supports 4k textures, we don't want to use them unless you have enough VRAM
		if (totalBudget < unsigned(FInt::RenderTextureManagerBudgetFor4k))
			return std::min(2048u, caps.maxTextureSize);
		else
			return caps.maxTextureSize;
	}
	else
	{
		// We limit all asssets we fetch by either 1024 or 512, depending on whether we may need to upscale
		// Upscaling to 1024 is bad since we have a lot of shirt/pants assets that are slightly larger than 512x512
		return caps.supportsTextureNPOT ? 1024 : 512;
	}
}

TextureManager::TextureData::TextureData()
	: orphaned(false)
	, orphanedPrev(0)
    , orphanedNext(0)
{
}

TextureRef TextureManager::TextureData::addExternalRef(const shared_ptr<Texture>& fallback)
{
	if (object.getStatus() == TextureRef::Status_Loaded)
	{
        // All refs should be equivalent so just return any object
        if (external.empty())
			external.push_back(object.clone());

		return external.back();
	}
	else
	{
        for (size_t i = 0; i < external.size(); ++i)
			if (external[i].getTexture() == fallback)
                return external[i];

		if (object.getStatus() == TextureRef::Status_Waiting)
            external.push_back(TextureRef::future(fallback));
		else
            external.push_back(fallback);

		return external.back();
	}
}

struct TextureRefUniquePredicate
{
    bool operator()(const TextureRef& ref) const
	{
		return ref.isUnique();
	}
};

void TextureManager::TextureData::removeUnusedExternalRefs()
{
	RBXASSERT(object.isUnique());

	external.erase(std::remove_if(external.begin(), external.end(), TextureRefUniquePredicate()), external.end());
}

void TextureManager::TextureData::updateAllRefsToLoaded(const shared_ptr<Texture>& texture, const ImageInfo& info)
{
    for (size_t i = 0; i < external.size(); ++i)
		external[i].updateAllRefsToLoaded(texture, info);

	object.updateAllRefsToLoaded(texture, info);
}

void TextureManager::TextureData::updateAllRefsToFailed()
{
    for (size_t i = 0; i < external.size(); ++i)
		external[i].updateAllRefsToFailed();

	object.updateAllRefsToFailed();
}

void TextureManager::TextureData::updateAllRefsToWaiting()
{
    for (size_t i = 0; i < external.size(); ++i)
        external[i].updateAllRefsToWaiting();

    object.updateAllRefsToWaiting();
}

TextureManager::TextureManager(VisualEngine* visualEngine)
    : visualEngine(visualEngine)
	, outstandingRequests(0)
    , liveCount(0)
    , liveSize(0)
    , orphanedCount(0)
    , orphanedSize(0)
    , orphanedHead(0)
    , orphanedTail(0)
	, gcSizeLast(0)
	, totalSizeBudget(0)
{
	loadingPool.reset(new ThreadPool(kTextureManagerThreads, BaseThreadPool::WaitForRunningTasks));
	pendingImages.reset(new rbx::safe_queue<LoadedImage>());

	fallbackTextures[Fallback_White] = createSingleColorTexture(255, 255, 255, 255);
	fallbackTextures[Fallback_Gray] = createSingleColorTexture(128, 128, 128, 255);
	fallbackTextures[Fallback_Black] = createSingleColorTexture(0, 0, 0, 255);
	fallbackTextures[Fallback_BlackTransparent] = createSingleColorTexture(0, 0, 0, 0);

    // Normal maps use BC3N encoding on all platforms except iOS
#ifdef RBX_PLATFORM_IOS
    fallbackTextures[Fallback_NormalMap] = createSingleColorTexture(128, 128, 255, 0);
#else
    fallbackTextures[Fallback_NormalMap] = createSingleColorTexture(255, 128, 0, 128);
#endif

	fallbackTextures[Fallback_Reflection] = createSingleColorTexture(0, 0, 0, 255, /* cube= */ true);

    // Base budget configuration
	totalSizeBudget = std::max(static_cast<unsigned int>(visualEngine->getRenderCaps()->getVidMemSize() / 3), kTextureManagerMinBudget);
    
    // Support budget overrides from config
    if (FInt::RenderTextureManagerBudget)
		totalSizeBudget = FInt::RenderTextureManagerBudget * 1024 * 1024;
}

TextureManager::~TextureManager()
{
}

void TextureManager::processPendingRequests()
{
    RBXPROFILER_SCOPE("Render", "TextureManager::processPendingRequests");

	unsigned int maxRequests = visualEngine->getSettings()->getEagerBulkExecution() ? ~0u : kTextureManagerRequestsPerFrame;

    LoadedImage li;
    unsigned int count = 0;

	while (pendingImages->pop_if_present(li))
	{
		RBXASSERT(outstandingRequests > 0);
		outstandingRequests--;

        Textures::iterator it = textures.find(li.id);
        if (it == textures.end()) continue;

        TextureData& data = it->second;

        // RBXASSERT(!data.orphaned); TODO: Why not to load orphaned textures? It makes sense for reloading... except, waiting for the texture to be actually used again, which is better
		RBXASSERT(data.object.getStatus() == TextureRef::Status_Waiting);

		if (Image* image = li.image.get())
		{
            try
            {
                Timer<Time::Precise> timer;

                shared_ptr<Texture> texture = createTexture(*image);

				texture->setDebugName(li.id.toString());

                FASTLOGS(FLog::Graphics, "Image loaded from %s", li.id.toString());
                FASTLOG5(FLog::Graphics, "Image dimensions %dx%d fmt %d  (decode %d ms, upload %d ms)", image->getWidth(), image->getHeight(), (int)texture->getFormat(), static_cast<int>(li.loadTime.msec()), static_cast<int>(timer.delta().msec()) );

				if (image->getWidth() != li.info.width || image->getHeight() != li.info.height)
					FASTLOG2(FLog::Graphics, "Image original dimensions %dx%d (had to rescale)", li.info.width, li.info.height);

				data.updateAllRefsToLoaded(texture, li.info);

				liveCount++;
				liveSize += getTextureSize(texture);
            }
            catch (const RBX::base_exception& e)
            {
                logError(li.id, li.context, e.what());

                data.updateAllRefsToFailed();
            }
        }
		else
		{
            data.updateAllRefsToFailed();
		}

        boost::unordered_set<ContentId>::iterator pendingReload = pendingReloads.find(li.id);
        if (pendingReload != pendingReloads.end())
        {
            pendingReloads.erase(pendingReload);
            reloadImage(li.id, li.context);
        }

		if (count++ >= maxRequests)
            break;
	}
}

void TextureManager::cancelPendingRequests()
{
	for (Textures::iterator it = textures.begin(); it != textures.end(); )
	{
		bool pending = (it->second.object.getStatus() == TextureRef::Status_Waiting);

		if (pending)
		{
			FASTLOGS(FLog::Graphics, "Cancelling image %s upon request", it->first.c_str());

			if (it->second.orphaned)
				removeFromOrphaned(&it->second);

			it = textures.erase(it);
		}
		else
			++it;
	}
}

void TextureManager::garbageCollectIncremental()
{
    RBXPROFILER_SCOPE("Render", "TextureManager::garbageCollectIncremental");

    // To catch up with allocation rate we need to visit the number of allocated elements since last run plus a small constant
	size_t visitCount = std::min(textures.size(), std::max(textures.size(), gcSizeLast) - gcSizeLast + 8);

	orphanUnusedTextures(visitCount);

    // Maintain total budget
	unsigned int totalSize = liveSize + orphanedSize;
	unsigned int maxOrphanedSize =
		(totalSize > totalSizeBudget)
            ? 0
			: std::min(totalSizeBudget - totalSize, std::min(totalSizeBudget / 4, kTextureManagerOrphanedBudgetLimit));

	collectOrphanedTextures(maxOrphanedSize);

    gcSizeLast = textures.size();
}

void TextureManager::garbageCollectFull()
{
    // Everything unused has to go
	orphanUnusedTextures(textures.size());
	collectOrphanedTextures(/* maxOrphanedSize= */ 0);

    gcSizeLast = textures.size();
}

void TextureManager::reloadImage(const ContentId& id, const std::string& context)
{
    // we want to reload files that were loaded
    Textures::iterator it = textures.find(id);

    if (it != textures.end())
    {
        if (it->second.object.getStatus() == TextureRef::Status_Waiting)
        {
            pendingReloads.insert(id);
        }
        else if (loadAsync(id, context))
        {
            StandardOut::singleton()->printf(MESSAGE_INFO, "Reloading %s texture", id.c_str());
            it->second.updateAllRefsToWaiting();
        }
    }
}

TextureRef TextureManager::load(const ContentId& id, Fallback fallback,  const std::string& context)
{
    // Cache lookup
	Textures::iterator it = textures.find(id);

    if (it != textures.end())
	{
		TextureData& data = it->second;

		if (data.orphaned)
		{
			removeFromOrphaned(&data);

            // Update live stats
			size_t textureSize = getTextureSize(data.object.getTexture());

            liveCount++;
            liveSize += textureSize;
		}

		return data.addExternalRef(fallbackTextures[fallback]);
	}

	bool result = loadAsync(id, context);

    // Insert entry into the cache
	TextureData& data = textures[id];

    data.id = id;
	data.object = result ? TextureRef::future(fallbackTextures[fallback]) : TextureRef(fallbackTextures[fallback], TextureRef::Status_Failed);

	RBXASSERT(!data.orphaned);

	return data.addExternalRef(fallbackTextures[fallback]);
}

bool TextureManager::loadAsync(const ContentId& id, const std::string& context)
{
    // Convert id to either asset or http form
    ContentId loadId = id;
    loadId.convertToLegacyContent(GetBaseURL());

    const DeviceCaps& caps = visualEngine->getDevice()->getCaps();

	unsigned int maxTextureSize = getMaxTextureSize(caps, totalSizeBudget, id.isAsset());

    unsigned int flags =
        (caps.supportsTextureDXT ? 0 : Image::Load_DecodeDXT) |
        (caps.supportsTextureNPOT ? 0 : Image::Load_RoundToPOT) |
        (caps.colorOrderBGR ? Image::Load_OutputBGR : 0) |
		(caps.supportsTexturePartialMipChain ? 0 : Image::Load_ForceFullMipChain);

    bool useRetina = caps.retina;

    try
	{
        if (loadId.isAsset() || loadId.isAppContent())
        {
			outstandingRequests++;

			loadingPool->schedule(boost::bind(&TextureManager::loadImageFile, pendingImages, id, loadId, maxTextureSize, flags, useRetina, context));

            return true;
        }
		else if (loadId.isHttp() || loadId.isAssetId() || loadId.isRbxHttp() || loadId.isNamedAsset())
        {
			if (ContentProvider* cp = visualEngine->getContentProvider())
			{
                boost::function<void (shared_ptr<const std::string>)> loadCallback =
					boost::bind(&TextureManager::loadImageHttpCallback, weak_ptr<ThreadPool>(loadingPool), weak_ptr<rbx::safe_queue<LoadedImage> >(pendingImages), _1, id, maxTextureSize, flags, context);

                outstandingRequests++;

				cp->getContent(loadId, ContentProvider::PRIORITY_TEXTURE, boost::bind(httpCallback, _1, _2, _3, loadCallback), AsyncHttpQueue::AsyncInline);

                return true;
			}
			else
			{
                throw RBX::runtime_error("Fetching remote assets is not available");
			}
        }
        else
        {
			throw RBX::runtime_error("Unexpected URL");
		}
	}
	catch (const RBX::base_exception& e)
	{
		logError(id, context, e.what());

        return false;
	}
}

void TextureManager::orphanUnusedTextures(size_t visitCount)
{
    Textures::iterator it = textures.find(gcKeyNext);

    for (size_t i = 0; i < visitCount; ++i)
    {
        if (it == textures.end())
            it = textures.begin();

        TextureData& data = it->second;

        data.removeUnusedExternalRefs();

		if (!data.orphaned && data.external.empty() && data.object.getStatus() == TextureRef::Status_Loaded)
        {
            addToOrphanedTail(&data);

            size_t textureSize = getTextureSize(data.object.getTexture());

            RBXASSERT(liveCount > 0 && liveSize >= textureSize);
            liveCount--;
            liveSize -= textureSize;

            orphanedCount++;
            orphanedSize += textureSize;
        }

        ++it;
    }

    gcKeyNext = (it == textures.end()) ? ContentId() : it->first;
}

void TextureManager::collectOrphanedTextures(unsigned int maxOrphanedSize)
{
	while (orphanedSize > maxOrphanedSize)
	{
		RBXASSERT(orphanedHead && orphanedTail);

        // Remove element from the beginning of the orphaned list
		TextureData* data = orphanedHead;

		removeFromOrphaned(data);

        // Remove texture from cache
		Textures::iterator it = textures.find(data->id);
        RBXASSERT(it != textures.end());

        textures.erase(it);
	}
}

void TextureManager::addToOrphanedTail(TextureData* data)
{
	RBXASSERT(!data->orphaned && !data->orphanedPrev && !data->orphanedNext);

	data->orphaned = true;

    if (orphanedHead)
    {
        RBXASSERT(orphanedTail);

        data->orphanedPrev = orphanedTail;
        orphanedTail->orphanedNext = data;
        orphanedTail = data;
    }
    else
    {
        RBXASSERT(!orphanedTail);

        orphanedHead = data;
        orphanedTail = data;
    }
}

void TextureManager::removeFromOrphaned(TextureData* data)
{
	RBXASSERT(data->orphaned);
	RBXASSERT(data->object.getStatus() == TextureRef::Status_Loaded);
	
	if (data->orphanedPrev)
		data->orphanedPrev->orphanedNext = data->orphanedNext;
	else
	{
		RBXASSERT(orphanedHead == data);
		orphanedHead = data->orphanedNext;
	}

	if (data->orphanedNext)
		data->orphanedNext->orphanedPrev = data->orphanedPrev;
	else
	{
		RBXASSERT(orphanedTail == data);
		orphanedTail = data->orphanedPrev;
	}

	data->orphaned = false;
    data->orphanedPrev = 0;
    data->orphanedNext = 0;

	// Update stats
	size_t textureSize = getTextureSize(data->object.getTexture());

	RBXASSERT(orphanedCount > 0 && orphanedSize >= textureSize);
	orphanedCount--;
	orphanedSize -= textureSize;
}

shared_ptr<Texture> TextureManager::createSingleColorTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a, bool cube)
{
	shared_ptr<Texture> result = visualEngine->getDevice()->createTexture(cube ? Texture::Type_Cube : Texture::Type_2D, Texture::Format_RGBA8, 1, 1, 1, 1, Texture::Usage_Static);
    RBXASSERT(result);

    unsigned char data[] = {r, g, b, a};

	if (visualEngine->getDevice()->getCaps().colorOrderBGR)
        std::swap(data[0], data[2]);

    for (int i = 0; i < (cube ? 6 : 1); ++i)
        result->upload(i, 0, TextureRegion(0, 0, 1, 1), data, sizeof(data));

    return result;
}

shared_ptr<Texture> TextureManager::createTexture(const Image& image)
{
    shared_ptr<Texture> texture = visualEngine->getDevice()->createTexture(
        image.getType(), image.getFormat(), image.getWidth(), image.getHeight(), image.getDepth(), image.getMipLevels(), Texture::Usage_Static);

    unsigned int faces = (image.getType() == Texture::Type_Cube) ? 6 : 1;

    for (unsigned int face = 0; face < faces; ++face)
    {
        for (unsigned int mip = 0; mip < image.getMipLevels(); ++mip)
        {
            unsigned int mipWidth = Texture::getMipSide(image.getWidth(), mip);
            unsigned int mipHeight = Texture::getMipSide(image.getHeight(), mip);
            unsigned int mipDepth = Texture::getMipSide(image.getDepth(), mip);
            unsigned int mipSize = Texture::getImageSize(image.getFormat(), mipWidth, mipHeight) * mipDepth;

            const unsigned char* mipData = image.getMipData(face, mip);

            texture->upload(face, mip, TextureRegion(0, 0, 0, mipWidth, mipHeight, mipDepth), mipData, mipSize);
        }
    }

    return texture;
}

void TextureManager::loadImageHttpCallback(const weak_ptr<ThreadPool>& loadingPoolWeak, const weak_ptr<rbx::safe_queue<LoadedImage> >& pendingImagesWeak, const shared_ptr<const std::string>& content, const ContentId& id, unsigned int maxTextureSize, unsigned int flags, const std::string& context)
{
	shared_ptr<ThreadPool> loadingPool = loadingPoolWeak.lock();
	shared_ptr<rbx::safe_queue<LoadedImage> > pendingImages = pendingImagesWeak.lock();

    if (loadingPool && pendingImages)
	{
        if (content)
        {
            loadingPool->schedule(boost::bind(&TextureManager::loadImageHttp, pendingImages, id, content, maxTextureSize, flags, context));
        }
        else
        {
            loadImageError(pendingImages, id, "Request failed", context);
        }
	}
	else
	{
        FASTLOGS(FLog::Graphics, "Abandoning image %s because TextureManager is dead", id.c_str());
	}
}

static std::pair<std::string, int> findImageAsset(const ContentId& id, bool useRetina)
{
	if (useRetina)
	{
		const std::string& idStr = id.toString();
		std::string::size_type dot = idStr.find_last_of('.');

		if (dot != std::string::npos)
		{
			ContentId retinaId(idStr.substr(0, dot) + "@2x" + idStr.substr(dot));
            std::string retinaPath = ContentProvider::findAsset(retinaId);

			if (!retinaPath.empty())
				return std::make_pair(retinaPath, 2);
		}
	}

    return std::make_pair(ContentProvider::findAsset(id), 1);
}

void TextureManager::loadImageFile(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const ContentId& loadId, unsigned int maxTextureSize, unsigned int flags, bool useRetina, const std::string& context)
{
	std::pair<std::string, int> imageAsset = findImageAsset(loadId, useRetina);

	if (!imageAsset.first.empty())
	{
        std::ifstream stream(utf8_decode(imageAsset.first).c_str(), std::ios_base::in | std::ios_base::binary);
        loadImage(pendingImages, id, stream, maxTextureSize, flags, imageAsset.second, context);
	}
	else
	{
        loadImageError(pendingImages, id, "File not found", context);
	}
}

void TextureManager::loadImageHttp(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const shared_ptr<const std::string>& content, unsigned int maxTextureSize, unsigned int flags, const std::string& context)
{
    std::istringstream in(*content);

    loadImage(pendingImages, id, in, maxTextureSize, flags, 1, context);
}

void TextureManager::loadImage(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, std::istream& stream, unsigned int maxTextureSize, unsigned int flags, int scale, const std::string& context)
{
    try
	{
		Timer<Time::Precise> timer;

		Image::LoadResult lr = Image::load(stream, maxTextureSize, flags);

        // Queue texture for creation
		LoadedImage li;
		li.id = id;
		li.image = lr.image;
		li.info = lr.info;
		li.context = context;
		li.loadTime = timer.delta();

        // Adjust image sizes for retina versions of the images to match the original one
		li.info.width /= scale;
		li.info.height /= scale;

		pendingImages->push(li);
	}
	catch (const std::bad_alloc& e)
	{
		loadImageError(pendingImages, id, e.what(), context);
	}
    catch (const RBX::base_exception& e)
	{
		loadImageError(pendingImages, id, e.what(), context);
	}
}

void TextureManager::loadImageError(const shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const char* error, const std::string& context)
{
    logError(id, context, error);

    // Queue texture reference for updating to failed state
    LoadedImage li;
    li.id = id;

    pendingImages->push(li);
}

bool TextureManager::isFallbackTexture(const shared_ptr<Texture>& tex)
{
    for (unsigned i = 0; i < Fallback_Count; ++i)
    {
        if (tex.get() == fallbackTextures[i].get())
            return true;
    }
    return false;
}

TextureManagerStats TextureManager::getStatistics() const
{
	TextureManagerStats result = {};

	result.queuedCount = outstandingRequests;

	result.totalBudget = totalSizeBudget;

    result.liveCount = liveCount;
    result.liveSize = liveSize;

    result.orphanedCount = orphanedCount;
    result.orphanedSize = orphanedSize;

    return result;
}

}
}
