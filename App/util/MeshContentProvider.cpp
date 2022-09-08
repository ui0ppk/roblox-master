#include "stdafx.h"

#include "v8datamodel/MeshContentProvider.h"

#include "GfxBase/FileMeshData.h"

namespace RBX {
    
	const char* const sMeshContentProvider = "MeshContentProvider";
	MeshContentProvider::MeshContentProvider()
		:DescribedNonCreatable<MeshContentProvider, CacheableContentProvider, sMeshContentProvider, RBX::Reflection::ClassDescriptor::RUNTIME_LOCAL>(CACHE_ENFORCE_MEMORY_SIZE, 1024 * 1024 * 32)
	{
		setName(sMeshContentProvider);
	}

	TaskScheduler::StepResult MeshContentProvider::ProcessTask(const std::string& id, shared_ptr<const std::string> data)
	{
		if(data)
        {
            boost::shared_ptr<CacheableContentProvider::CachedItem> mesh(new CacheableContentProvider::CachedItem());

			mesh->data = ReadFileMesh(*data);

			mesh->requestResult = AsyncHttpQueue::Succeeded;
			updateContent(id,mesh);

			--pendingRequests;
			return TaskScheduler::Stepped;
		}
		else{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "MeshContentProvider failed to process %s because 'could not fetch'", id.c_str());
		}
		markContentFailed(id);
		return TaskScheduler::Stepped;
	}

	void MeshContentProvider::updateContent(const std::string& id, boost::shared_ptr<CacheableContentProvider::CachedItem> mesh)
	{
		if (mesh->data)
		{
			boost::shared_ptr<FileMeshData> meshData = boost::static_pointer_cast<FileMeshData>(mesh->data);
			lruCache->insert(id, mesh, meshData->faces.size() * sizeof(meshData->faces[0]) + meshData->vnts.size() * sizeof(meshData->vnts[0]));
		}
		else
			lruCache->insert(id, mesh, 0);
	}

}



