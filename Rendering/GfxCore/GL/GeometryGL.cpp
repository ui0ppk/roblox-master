#include "GeometryGL.h"

#include "DeviceGL.h"

#include "HeadersGL.h"

namespace RBX
{
namespace Graphics
{

static const GLenum gBufferUsageGL[GeometryBuffer::Usage_Count] =
{
    GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW
};

static const GLenum gBufferLockGL[GeometryBuffer::Lock_Count] =
{
	GL_MAP_WRITE_BIT,
	GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
};

struct VertexFormatGL
{
    int size;
    GLenum type;
    bool normalized;
};

static const VertexFormatGL gVertexFormatGL[VertexLayout::Format_Count] =
{
	{ 1, GL_FLOAT, false },
	{ 2, GL_FLOAT, false },
	{ 3, GL_FLOAT, false },
	{ 4, GL_FLOAT, false },
	{ 2, GL_SHORT, false },
	{ 4, GL_SHORT, false },
	{ 4, GL_UNSIGNED_BYTE, false },
	{ 4, GL_UNSIGNED_BYTE, true },
};

static const GLenum gGeometryPrimitiveGL[Geometry::Primitive_Count] =
{
    GL_TRIANGLES,
    GL_LINES,
	GL_POINTS,
	GL_TRIANGLE_STRIP
};

static unsigned int getVertexAttributeGL(VertexLayout::Semantic semantic, unsigned int index)
{
    switch (semantic)
	{
	case VertexLayout::Semantic_Position:
		RBXASSERT(index == 0);
        return 0;

	case VertexLayout::Semantic_Normal:
		RBXASSERT(index == 0);
        return 1;

	case VertexLayout::Semantic_Color:
		RBXASSERT(index < 2);
        return 2 + index;

	case VertexLayout::Semantic_Texture:
        RBXASSERT(index < 8);
        return 4 + index;

	default:
		RBXASSERT(false);
        return -1;
	}
}

const char* VertexLayoutGL::getShaderAttributeName(unsigned int id)
{
    static const char* table[] =
	{
        "vertex",
        "normal",
        "colour",
        "secondary_colour",
        "uv0",
        "uv1",
        "uv2",
        "uv3",
        "uv4",
        "uv5",
        "uv6",
        "uv7",
	};

    return (id < ARRAYSIZE(table)) ? table[id] : NULL;
}

VertexLayoutGL::VertexLayoutGL(Device* device, const std::vector<Element>& elements)
    : VertexLayout(device, elements)
{
}

VertexLayoutGL::~VertexLayoutGL()
{
}

template <typename Base> GeometryBufferGL<Base>::GeometryBufferGL(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage, unsigned int target)
    : Base(device, elementSize, elementCount, usage)
    , target(target)
    , id(0)
    , locked(0)
{
}

template <typename Base> void GeometryBufferGL<Base>::create()
{
	glGenBuffers(1, &id);
    RBXASSERT(id);

	glBindBuffer(target, id);
	glBufferData(target, this->elementSize * this->elementCount, NULL, gBufferUsageGL[this->usage]);
}

template <typename Base> GeometryBufferGL<Base>::~GeometryBufferGL()
{
    RBXASSERT(!locked);

	glDeleteBuffers(1, &id);
}

template <typename Base> void* GeometryBufferGL<Base>::lock(GeometryBuffer::LockMode mode)
{
    RBXASSERT(!locked);
    
    unsigned int size = this->elementSize * this->elementCount;

    const DeviceCapsGL& caps = static_cast<DeviceGL*>(this->device)->getCapsGL();

	if (caps.extMapBuffer)
	{
        glBindBuffer(target, id);

		if (caps.extMapBufferRange)
		{
			locked = glMapBufferRange(target, 0, size, gBufferLockGL[mode]);
		}
		else
		{
            if (mode == GeometryBuffer::Lock_Discard)
                glBufferData(target, size, NULL, gBufferUsageGL[this->usage]);

            locked = glMapBuffer(target, GL_WRITE_ONLY);
		}
	}
	else
	{
        locked = new char[size];
	}

    RBXASSERT(locked);

    return locked;  
}

template <typename Base> void GeometryBufferGL<Base>::unlock()
{
    RBXASSERT(locked);

    const DeviceCapsGL& caps = static_cast<DeviceGL*>(this->device)->getCapsGL();

	if (caps.extMapBuffer)
	{
        glBindBuffer(target, id);
        glUnmapBuffer(target);
	}
	else
	{
        glBindBuffer(target, id);
		glBufferSubData(target, 0, this->elementSize * this->elementCount, locked);

        delete[] static_cast<char*>(locked);
	}

    locked = NULL;
}

template <typename Base> void GeometryBufferGL<Base>::upload(unsigned int offset, const void* data, unsigned int size)
{
    RBXASSERT(!locked);
    RBXASSERT(offset + size <= this->elementSize * this->elementCount);

    glBindBuffer(target, id);
    glBufferSubData(target, offset, size, data);
}

VertexBufferGL::VertexBufferGL(Device* device, size_t elementSize, size_t elementCount, Usage usage)
	: GeometryBufferGL<VertexBuffer>(device, elementSize, elementCount, usage, GL_ARRAY_BUFFER)
{
    create();
}

VertexBufferGL::~VertexBufferGL()
{
}

IndexBufferGL::IndexBufferGL(Device* device, size_t elementSize, size_t elementCount, Usage usage)
	: GeometryBufferGL<IndexBuffer>(device, elementSize, elementCount, usage, GL_ELEMENT_ARRAY_BUFFER)
{
    const DeviceCapsGL& caps = static_cast<DeviceGL*>(device)->getCapsGL();

	if (elementSize != 2 && elementSize != 4)
		throw RBX::runtime_error("Invalid element size: %d", (int)elementSize);

	if (elementSize == 4 && !caps.supportsIndex32)
		throw RBX::runtime_error("Unsupported element size: %d", (int)elementSize);

	create();
}

IndexBufferGL::~IndexBufferGL()
{
}

GeometryGL::GeometryGL(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
	: Geometry(device, layout, vertexBuffers, indexBuffer, baseVertexIndex)
    , id(0)
	, indexElementSize(0)
{
    const DeviceCapsGL& caps = static_cast<DeviceGL*>(device)->getCapsGL();

	if (caps.extVertexArrayObject)
	{
		glGenVertexArrays(1, &id);
		RBXASSERT(id);

		glBindVertexArray(id);

		bindArrays();

		glBindVertexArray(0);
	}

	if (indexBuffer)
	{
        // Cache indexElementSize to reduce cache misses in case we're using VAO
		indexElementSize = indexBuffer->getElementSize();
	}
}

GeometryGL::~GeometryGL()
{
    if (id)
	{
		glDeleteVertexArrays(1, &id);
	}
}

void GeometryGL::draw(Primitive primitive, unsigned int offset, unsigned int count)
{
    unsigned int mask = 0;

    // Setup vertex/index buffers
    if (id)
		glBindVertexArray(id);
	else
		mask = bindArrays();

    // Perform draw call
    if (indexBuffer)
	{
		GLenum indexElementType = indexElementSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

		glDrawElements(gGeometryPrimitiveGL[primitive], count, indexElementType, reinterpret_cast<void*>(offset * indexElementSize));
	}
	else
		glDrawArrays(gGeometryPrimitiveGL[primitive], offset, count);

    // Unbind buffers to prevent draw call interference
    if (id)
		glBindVertexArray(0);
	else
		unbindArrays(mask);
}

unsigned int GeometryGL::bindArrays()
{
    unsigned int mask = 0;
    unsigned int vbId = 0;

	const std::vector<VertexLayout::Element>& elements = layout->getElements();

    for (size_t i = 0; i < elements.size(); ++i)
	{
		const VertexLayout::Element& e = elements[i];

		int index = getVertexAttributeGL(e.semantic, e.semanticIndex);
        RBXASSERT(index >= 0);

		RBXASSERT(e.stream < vertexBuffers.size());

		VertexBufferGL* vb = static_cast<VertexBufferGL*>(vertexBuffers[e.stream].get());

		RBXASSERT(e.offset < vb->getElementSize());
            
        size_t stride = vb->getElementSize();
		size_t offset = e.offset + stride * baseVertexIndex;
            
		const VertexFormatGL& formatGl = gVertexFormatGL[e.format];
            
		if (vbId != vb->getId())
		{
			vbId = vb->getId();

            glBindBuffer(GL_ARRAY_BUFFER, vbId);
		}

        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, formatGl.size, formatGl.type, formatGl.normalized, stride, reinterpret_cast<void*>(offset));

        mask |= 1 << index;
    }

	if (indexBuffer)
	{
        IndexBufferGL* ib = static_cast<IndexBufferGL*>(indexBuffer.get());

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->getId());
	}

    return mask;
}

void GeometryGL::unbindArrays(unsigned int mask)
{
    for (int i = 0; i < 16; ++i)
        if (mask & (1 << i))
			glDisableVertexAttribArray(i);
}

// Instantiate GeometryBufferGL template
template class GeometryBufferGL<VertexBuffer>;
template class GeometryBufferGL<IndexBuffer>;

}
}
