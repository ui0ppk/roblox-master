#pragma once

#include <stddef.h>

namespace RBX {

class Image
{
public:
	virtual ~Image() {}

	virtual size_t getSize() const = 0;
	
	virtual int getOriginalWidth() const = 0;
	virtual int getOriginalHeight() const = 0;
};

}


