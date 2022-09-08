#pragma once

#include "GfxCore/Geometry.h"

struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

namespace RBX
{
namespace Graphics
{

class VertexLayoutD3D9: public VertexLayout
{
public:
    VertexLayoutD3D9(Device* device, const std::vector<Element>& elements);
    ~VertexLayoutD3D9();

	IDirect3DVertexDeclaration9* getObject() const { return object; }

private:
	IDirect3DVertexDeclaration9* object;
};

class VertexBufferD3D9: public VertexBuffer
{
public:
    VertexBufferD3D9(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~VertexBufferD3D9();
    
    virtual void* lock(GeometryBuffer::LockMode mode);
    virtual void unlock();

    virtual void upload(unsigned int offset, const void* data, unsigned int size);

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

	IDirect3DVertexBuffer9* getObject() const { return object; }

private:
	IDirect3DVertexBuffer9* object;
};

class IndexBufferD3D9: public IndexBuffer
{
public:
    IndexBufferD3D9(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~IndexBufferD3D9();
    
    virtual void* lock(GeometryBuffer::LockMode mode);
    virtual void unlock();

    virtual void upload(unsigned int offset, const void* data, unsigned int size);

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

	IDirect3DIndexBuffer9* getObject() const { return object; }

private:
	IDirect3DIndexBuffer9* object;
};

class GeometryD3D9: public Geometry
{
public:
    GeometryD3D9(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);
    ~GeometryD3D9();

	void draw(Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd, VertexLayoutD3D9** layoutCache, GeometryD3D9** geometryCache);

private:
};

}
}
