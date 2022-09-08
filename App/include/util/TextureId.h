#pragma once

#include "Util/ContentId.h"

namespace RBX {

	class TextureId : public ContentId
	{
	public:
		TextureId(const ContentId& id):ContentId(id) {}
		TextureId(const char* id):ContentId(id) {}
		TextureId(const std::string& id):ContentId(id) {}
		TextureId() {}

		static TextureId nullTexture() {
			static TextureId t;				// note - the name in the contentId will get a boost call_once
			return t;
		}
	};
}