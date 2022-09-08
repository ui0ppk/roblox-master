#pragma once

#include "ImageInfo.h"

#include <rbx/Boost.hpp>

namespace RBX
{
namespace Graphics
{

class Texture;

class TextureRef
{
public:
    enum Status
	{
        Status_Null,
        Status_Waiting,
        Status_Failed,
        Status_Loaded
	};

    static TextureRef future(const shared_ptr<Texture>& fallback);

    TextureRef()
	{
	}

	TextureRef(const shared_ptr<Texture>& texture, Status status = Status_Loaded);

    TextureRef clone() const;

    void updateAllRefs(const shared_ptr<Texture>& texture);
    void updateAllRefsToLoaded(const shared_ptr<Texture>& texture);
    void updateAllRefsToLoaded(const shared_ptr<Texture>& texture, const ImageInfo& info);
    void updateAllRefsToFailed();
    void updateAllRefsToWaiting();

    const shared_ptr<Texture>& getTexture() const
	{
        return data ? data->texture : emptyTexture;
	}

    bool isUnique() const
	{
		return data.unique();
	}

    Status getStatus() const
	{
		return data ? data->status : Status_Null;
	}

    const ImageInfo& getInfo() const
	{
        return data ? data->info : emptyInfo;
	}

private:
    struct Data
	{
        shared_ptr<Texture> texture;
        Status status;
        ImageInfo info;
	};

    shared_ptr<Data> data;

    static shared_ptr<Texture> emptyTexture;
    static ImageInfo emptyInfo;

	TextureRef(Data* data);
};

}
}
