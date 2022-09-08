#pragma once

#include "GfxCore/Geometry.h"

namespace RBX
{
namespace Graphics
{

class VertexLayoutGL: public VertexLayout
{
public:
    static const char* getShaderAttributeName(unsigned int id);

    VertexLayoutGL(Device* device, const std::vector<Element>& elements);
    ~VertexLayoutGL();
};

template <typename Base> class GeometryBufferGL: public Base
{
public:
    GeometryBufferGL(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage, unsigned int target);
	~GeometryBufferGL();

    virtual void* lock(GeometryBuffer::LockMode mode);
    virtual void unlock();

    virtual void upload(unsigned int offset, const void* data, unsigned int size);

    unsigned int getId() const { return id; }

protected:
    void create();

private:
    unsigned int target;
	unsigned int id;

    void* locked;
};

class VertexBufferGL: public GeometryBufferGL<VertexBuffer>
{
public:
    VertexBufferGL(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~VertexBufferGL();
};

class IndexBufferGL: public GeometryBufferGL<IndexBuffer>
{
public:
    IndexBufferGL(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~IndexBufferGL();
};

class GeometryGL: public Geometry
{
public:
    GeometryGL(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);
    ~GeometryGL();

    void draw(Primitive primitive, unsigned int offset, unsigned int count);

    unsigned int getId() const { return id; }

private:
    unsigned int id;
    unsigned int indexElementSize;

    unsigned int bindArrays();
    void unbindArrays(unsigned int mask);
};

}
}
