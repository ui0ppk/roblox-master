#include "GeometryD3D11.h"

#include "DeviceD3D11.h"
#include "ShaderD3D11.h"

#include "HeadersD3D11.h"

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{
    struct BufferUsageD3D11
    {
        D3D11_USAGE usage;
        unsigned cpuAccess;
    };

    static const BufferUsageD3D11 gBufferUsageD3D11[GeometryBuffer::Usage_Count] =
    {
        { D3D11_USAGE_DEFAULT, 0 },
        { D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE },
    };

    static const DXGI_FORMAT gVertexLayoutFormatD3D11[VertexLayout::Format_Count] =
    {
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT,
        DXGI_FORMAT_R32G32B32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R16G16B16A16_SINT,
        DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_UNORM,
    };

    static const LPCSTR gVertexLayoutSemanticD3D11[VertexLayout::Semantic_Count] =
    {
        "POSITION",
        "NORMAL",
        "COLOR",
        "TEXCOORD",
    };

    static const D3D11_PRIMITIVE_TOPOLOGY gGeometryPrimitiveD3D11[Geometry::Primitive_Count] =
    {
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
        D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
    };

    static const D3D11_MAP gLockModeD3D11[GeometryBuffer::Lock_Count] =
    {
        D3D11_MAP_WRITE,
        D3D11_MAP_WRITE_DISCARD
    };

    VertexLayoutD3D11::VertexLayoutD3D11(Device* device, const std::vector<Element>& elements)
        : VertexLayout(device, elements)
    {
        
        for (size_t i = 0; i < elements.size(); ++i)
        {
            const Element& e = elements[i];
            D3D11_INPUT_ELEMENT_DESC e11 = {};

            e11.InputSlot = e.stream;
            e11.AlignedByteOffset = e.offset;
            e11.SemanticName = gVertexLayoutSemanticD3D11[e.semantic];
            e11.Format = gVertexLayoutFormatD3D11[e.format];
            e11.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            e11.InstanceDataStepRate = 0;
            e11.SemanticIndex = e.semanticIndex;

            elements11.push_back(e11);
        }
    }

    VertexLayoutD3D11::~VertexLayoutD3D11()
    {
        for (size_t i = 0; i < shaders.size(); ++i)
        {
            shared_ptr<VertexShaderD3D11> vertexShader = shaders[i].lock();
            if (vertexShader)
                vertexShader->removeLayout(this);
        }

        static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedVertexLayout();
    }

    void VertexLayoutD3D11::registerShader(const shared_ptr<VertexShaderD3D11>& shader)
	{
		shaders.push_back(weak_ptr<VertexShaderD3D11>(shader));
	}

    template <typename Base> GeometryBufferD3D11<Base>::GeometryBufferD3D11(Device* device, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
        : Base(device, elementSize, elementCount, usage)
        , locked(0)
        , object(NULL)
    {
    }

    template <typename Base> void GeometryBufferD3D11<Base>::create(unsigned bindFlags)
    {
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = gBufferUsageD3D11[usage].usage; 
        bd.ByteWidth = elementSize * elementCount;
        bd.BindFlags = bindFlags;
        bd.CPUAccessFlags = gBufferUsageD3D11[usage].cpuAccess;
        bd.MiscFlags = 0;
        bd.StructureByteStride = 0;

        HRESULT hr = device11->CreateBuffer( &bd, NULL, &object );
        if (FAILED(hr))
            throw RBX::runtime_error("Couldn't create geometry buffer: %x", hr);
    }

    template <typename Base> GeometryBufferD3D11<Base>::~GeometryBufferD3D11()
    {
        RBXASSERT(!locked);

        ReleaseCheck(object);
    }

    template <typename Base> void* GeometryBufferD3D11<Base>::lock(GeometryBuffer::LockMode mode)
    {
        RBXASSERT(!locked);

        if (usage == Usage::Usage_Static)
        {
            locked = new char[elementSize * elementCount];
        }
        else
        {
            ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();
            D3D11_MAP mapMode = gLockModeD3D11[mode];

            D3D11_MAPPED_SUBRESOURCE resource;
            HRESULT hr = context11->Map(object, 0, mapMode, 0, &resource);
            if (FAILED(hr))
            {
                FASTLOG2(FLog::Graphics, "Failed to lock VB (size %d): %x", elementCount * elementSize, hr);
                return NULL;
            }

            locked = resource.pData;
        }

        RBXASSERT(locked);
        return locked;
    }

    template <typename Base> void GeometryBufferD3D11<Base>::unlock()
    {
        RBXASSERT(locked);

        if (usage == Usage::Usage_Static)
        {
            upload(0, locked, elementCount * elementSize);
            delete[] static_cast<char*>(locked);
        }
        else
        {
            ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

            context11->Unmap(object, 0);
        }

        locked = NULL;
    }

    template <typename Base> void GeometryBufferD3D11<Base>::upload(unsigned int offset, const void* data, unsigned int size)
    {
        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

        D3D11_BOX box;
        box.left = offset;
        box.right = offset + size;
        box.top = 0;
        box.bottom = 1;
        box.front = 0;
        box.back = 1;

        context11->UpdateSubresource(object, 0, &box, data, 0, 0);
    }

    VertexBufferD3D11::VertexBufferD3D11(Device* device, size_t elementSize, size_t elementCount, Usage usage)
        : GeometryBufferD3D11<VertexBuffer>(device, elementSize, elementCount, usage)
    {
        create(D3D11_BIND_VERTEX_BUFFER);
    }

    VertexBufferD3D11::~VertexBufferD3D11()
    {
    }

    IndexBufferD3D11::IndexBufferD3D11(Device* device, size_t elementSize, size_t elementCount, Usage usage)
        : GeometryBufferD3D11<IndexBuffer>(device, elementSize, elementCount, usage)
    {
        if (elementSize != 2 && elementSize != 4)
            throw RBX::runtime_error("Invalid element size: %d", (int)elementSize);

        create(D3D11_BIND_INDEX_BUFFER);
    }

    IndexBufferD3D11::~IndexBufferD3D11()
    {
    }

    GeometryD3D11::GeometryD3D11(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
        : Geometry(device, layout, vertexBuffers, indexBuffer, baseVertexIndex)
    {
    }

    GeometryD3D11::~GeometryD3D11()
    {
        static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedGeometry();
    }

    void GeometryD3D11::draw(Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd, VertexLayoutD3D11** layoutCache, GeometryD3D11** geometryCache, ShaderProgramD3D11** programCache)
    {
        RBXASSERT(*programCache);

        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();
        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

        if (*layoutCache != layout.get())
        {
            VertexLayoutD3D11* vertexLayout = static_cast<VertexLayoutD3D11*>(layout.get());
            *layoutCache = vertexLayout;

            ID3D11InputLayout* inputLayout11 = (*programCache)->getInputLayout11(vertexLayout);
            context11->IASetInputLayout(inputLayout11);
        }

        if (*geometryCache != this)
        {
            *geometryCache = this;

            for (size_t i = 0; i < vertexBuffers.size(); ++i)
            {
                VertexBufferD3D11* vb = static_cast<VertexBufferD3D11*>(vertexBuffers[i].get());
                ID3D11Buffer* vb11 = vb->getObject();
                unsigned int offsetVB = 0;
                unsigned int stride = vb->getElementSize();
                context11->IASetVertexBuffers(i, 1, &vb11, &stride, &offsetVB);
            }

            if (indexBuffer)
            {
                DXGI_FORMAT format = indexBuffer->getElementSize() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
                context11->IASetIndexBuffer(static_cast<IndexBufferD3D11*>(indexBuffer.get())->getObject(), format, 0);
            }
        }

        (*programCache)->uploadConstantBuffers();

        context11->IASetPrimitiveTopology(gGeometryPrimitiveD3D11[primitive]);

        if (indexBuffer)
            context11->DrawIndexed(count, offset, baseVertexIndex);
        else
            context11->Draw(count, offset);
    }


}
}
