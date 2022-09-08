#pragma once

#include <boost/shared_ptr.hpp>
#include "boost/enable_shared_from_this.hpp"
#include "g3d/Vector2.h"
#include <string>

namespace RBX {
	typedef boost::shared_ptr<class TextureProxyBase> TextureProxyBaseRef;

class TextureProxyBase : public boost::enable_shared_from_this<TextureProxyBase> 
{
private:
	typedef boost::enable_shared_from_this<TextureProxyBase> Super;

public:
	TextureProxyBase() {}
	virtual ~TextureProxyBase() {}
	
	virtual G3D::Vector2 getOriginalSize() = 0;

	static const unsigned int numStrips = 32;
	static float stripWidth() {
		return  1.0f / (float) numStrips;
	}
};

} // namespace
