#pragma once

#include "GfxCore/Geometry.h"

struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11DeviceContext;
struct D3D11_INPUT_ELEMENT_DESC;

namespace RBX
{
namespace Graphics
{

class ShaderProgramD3D11;
class VertexShaderD3D11;

class VertexLayoutD3D11: public VertexLayout
{
public:
    VertexLayoutD3D11(Device* device, const std::vector<Element>& elements);
    ~VertexLayoutD3D11();
    
    const D3D11_INPUT_ELEMENT_DESC* getElements11() const { return elements11.data(); }
    size_t getElementsCount() const { return elements11.size(); }

    void registerShader(const shared_ptr<VertexShaderD3D11>& shader);

private:
    std::vector<D3D11_INPUT_ELEMENT_DESC> elements11;
    std::vector<weak_ptr<VertexShaderD3D11>> shaders; // shaders having this vertex declaration
};

template <typename Base> class GeometryBufferD3D11: public Base
{
public:
    GeometryBufferD3D11(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage);
    ~GeometryBufferD3D11();

    virtual void* lock(GeometryBuffer::LockMode mode);
    virtual void unlock();

    virtual void upload(unsigned int offset, const void* data, unsigned int size);

    ID3D11Buffer* getObject() const { return object; }

protected:
    void create(unsigned bindFlags);

private:
    ID3D11Buffer* object;

    void* locked;
};


class VertexBufferD3D11: public GeometryBufferD3D11<VertexBuffer>
{
public:
    VertexBufferD3D11(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~VertexBufferD3D11();
};

class IndexBufferD3D11: public GeometryBufferD3D11<IndexBuffer>
{
public:
    IndexBufferD3D11(Device* device, size_t elementSize, size_t elementCount, Usage usage);
    ~IndexBufferD3D11();
};

class GeometryD3D11: public Geometry
{
public:
    GeometryD3D11(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);
    ~GeometryD3D11();

	void draw(Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd, VertexLayoutD3D11** layoutCache, GeometryD3D11** geometryCache, ShaderProgramD3D11** programCache);

private:
};

}
}
