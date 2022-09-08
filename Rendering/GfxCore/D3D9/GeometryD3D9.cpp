#include "GeometryD3D9.h"

#include "DeviceD3D9.h"

#include <d3d9.h>

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{

static const D3DDECLTYPE gVertexLayoutFormatD3D9[VertexLayout::Format_Count] =
{
	D3DDECLTYPE_FLOAT1,
	D3DDECLTYPE_FLOAT2,
	D3DDECLTYPE_FLOAT3,
	D3DDECLTYPE_FLOAT4,
	D3DDECLTYPE_SHORT2,
	D3DDECLTYPE_SHORT4,
	D3DDECLTYPE_UBYTE4,
	D3DDECLTYPE_D3DCOLOR,
};

static const D3DDECLUSAGE gVertexLayoutSemanticD3D9[VertexLayout::Semantic_Count] =
{
	D3DDECLUSAGE_POSITION,
	D3DDECLUSAGE_NORMAL,
	D3DDECLUSAGE_COLOR,
	D3DDECLUSAGE_TEXCOORD,
};

struct BufferUsageD3D9
{
    D3DPOOL pool;
    DWORD usage;
};

static const BufferUsageD3D9 gBufferUsageD3D9[GeometryBuffer::Usage_Count] =
{
	{ D3DPOOL_MANAGED, 0 },
	{ D3DPOOL_DEFAULT, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY },
};

static const DWORD gBufferLockD3D9[GeometryBuffer::Usage_Count][GeometryBuffer::Lock_Count] =
{
    { 0, 0 },
	{ 0, D3DLOCK_DISCARD },
};

static const D3DPRIMITIVETYPE gGeometryPrimitiveD3D9[Geometry::Primitive_Count] =
{
	D3DPT_TRIANGLELIST,
	D3DPT_LINELIST,
	D3DPT_POINTLIST,
	D3DPT_TRIANGLESTRIP
};

static IDirect3DVertexBuffer9* createVertexBuffer(IDirect3DDevice9* device9, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
{
	IDirect3DVertexBuffer9* result = NULL;

	HRESULT hr = device9->CreateVertexBuffer(elementSize * elementCount, gBufferUsageD3D9[usage].usage, 0, gBufferUsageD3D9[usage].pool, &result, NULL);
    if (FAILED(hr))
		throw RBX::runtime_error("Error creating vertex buffer: %x", hr);

    return result;
}

static IDirect3DIndexBuffer9* createIndexBuffer(IDirect3DDevice9* device9, size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
{
	IDirect3DIndexBuffer9* result = NULL;

	D3DFORMAT format = (elementSize == 2) ? D3DFMT_INDEX16 : D3DFMT_INDEX32;

	HRESULT hr = device9->CreateIndexBuffer(elementSize * elementCount, gBufferUsageD3D9[usage].usage, format, gBufferUsageD3D9[usage].pool, &result, NULL);
    if (FAILED(hr))
		throw RBX::runtime_error("Error creating index buffer: %x", hr);

    return result;
}

VertexLayoutD3D9::VertexLayoutD3D9(Device* device, const std::vector<Element>& elements)
    : VertexLayout(device, elements)
    , object(NULL)
{
    const DeviceCapsD3D9& caps = static_cast<DeviceD3D9*>(device)->getCapsD3D9();

	std::vector<D3DVERTEXELEMENT9> elements9;

    for (size_t i = 0; i < elements.size(); ++i)
	{
        const Element& e = elements[i];
		D3DVERTEXELEMENT9 e9;

		e9.Stream = e.stream;
		e9.Offset = e.offset;
		e9.Type = gVertexLayoutFormatD3D9[e.format];
		e9.Method = D3DDECLMETHOD_DEFAULT;
		e9.Usage = gVertexLayoutSemanticD3D9[e.semantic];
		e9.UsageIndex = e.semanticIndex;

        // FFP does not support UBYTE4 but we won't use it anyway so convert to a float
		if (caps.supportsFFP && e9.Type == D3DDECLTYPE_UBYTE4)
		{
			RBXASSERT(e9.Usage == D3DDECLUSAGE_TEXCOORD);
			e9.Type = D3DDECLTYPE_FLOAT1;
		}

		elements9.push_back(e9);
	}

	D3DVERTEXELEMENT9 elementsEnd = D3DDECL_END();

	elements9.push_back(elementsEnd);

	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	HRESULT hr = device9->CreateVertexDeclaration(&elements9[0], &object);
    if (FAILED(hr))
		throw RBX::runtime_error("Error creating vertex declaration: %x", hr);
}

VertexLayoutD3D9::~VertexLayoutD3D9()
{
	static_cast<DeviceD3D9*>(device)->getImmediateContextD3D9()->invalidateCachedVertexLayout();

    object->Release();
}

VertexBufferD3D9::VertexBufferD3D9(Device* device, size_t elementSize, size_t elementCount, Usage usage)
	: VertexBuffer(device, elementSize, elementCount, usage)
    , object(NULL)
{
	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

    object = createVertexBuffer(device9, elementSize, elementCount, usage);
}

VertexBufferD3D9::~VertexBufferD3D9()
{
    if (object)
        object->Release();
}

void* VertexBufferD3D9::lock(GeometryBuffer::LockMode mode)
{
    void* result;

	HRESULT hr = object->Lock(0, 0, &result, gBufferLockD3D9[usage][mode]);

	if (SUCCEEDED(hr))
        return result;
    else
	{
		FASTLOG2(FLog::Graphics, "Failed to lock VB (size %d): %x", elementCount * elementSize, hr);
        return NULL;
	}
}

void VertexBufferD3D9::unlock()
{
	object->Unlock();
}

void VertexBufferD3D9::upload(unsigned int offset, const void* data, unsigned int size)
{
    RBXASSERT(offset + size <= elementSize * elementCount);

	void* target = lock(Lock_Normal);
    memcpy(static_cast<char*>(target) + offset, data, size);
    unlock();
}

void VertexBufferD3D9::onDeviceLost()
{
	if (object && gBufferUsageD3D9[usage].pool == D3DPOOL_DEFAULT)
	{
		object->Release();
        object = NULL;
	}
}

void VertexBufferD3D9::onDeviceRestored()
{
	if (gBufferUsageD3D9[usage].pool == D3DPOOL_DEFAULT)
	{
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

		object = createVertexBuffer(device9, elementSize, elementCount, usage);
	}
}

IndexBufferD3D9::IndexBufferD3D9(Device* device, size_t elementSize, size_t elementCount, Usage usage)
	: IndexBuffer(device, elementSize, elementCount, usage)
{
	if (elementSize != 2 && elementSize != 4)
		throw RBX::runtime_error("Invalid element size: %d", (int)elementSize);

    IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

    object = createIndexBuffer(device9, elementSize, elementCount, usage);
}

IndexBufferD3D9::~IndexBufferD3D9()
{
    if (object)
        object->Release();
}

void* IndexBufferD3D9::lock(GeometryBuffer::LockMode mode)
{
    void* result;

	HRESULT hr = object->Lock(0, 0, &result, gBufferLockD3D9[usage][mode]);

	if (SUCCEEDED(hr))
        return result;
    else
	{
		FASTLOG2(FLog::Graphics, "Failed to lock IB (size %d): %x", elementCount * elementSize, hr);
        return NULL;
	}
}

void IndexBufferD3D9::unlock()
{
	object->Unlock();
}

void IndexBufferD3D9::upload(unsigned int offset, const void* data, unsigned int size)
{
    RBXASSERT(offset + size <= elementSize * elementCount);

	void* target = lock(Lock_Normal);
    memcpy(static_cast<char*>(target) + offset, data, size);
    unlock();
}

void IndexBufferD3D9::onDeviceLost()
{
	if (object && gBufferUsageD3D9[usage].pool == D3DPOOL_DEFAULT)
	{
		object->Release();
        object = NULL;
	}
}

void IndexBufferD3D9::onDeviceRestored()
{
	if (gBufferUsageD3D9[usage].pool == D3DPOOL_DEFAULT)
	{
        IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

		object = createIndexBuffer(device9, elementSize, elementCount, usage);
	}
}


GeometryD3D9::GeometryD3D9(Device* device, const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
	: Geometry(device, layout, vertexBuffers, indexBuffer, baseVertexIndex)
{
}

GeometryD3D9::~GeometryD3D9()
{
	static_cast<DeviceD3D9*>(device)->getImmediateContextD3D9()->invalidateCachedGeometry();
}

static unsigned int getPrimitiveCount(Geometry::Primitive primitive, unsigned int count)
{
    switch (primitive)
	{
	case Geometry::Primitive_Triangles:
        return count / 3;

	case Geometry::Primitive_Lines:
        return count / 2;

    case Geometry::Primitive_Points:
        return count;

    case Geometry::Primitive_TriangleStrip:
        return count - 2;

	default:
		RBXASSERT(false);
        return 0;
	}
}

void GeometryD3D9::draw(Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd, VertexLayoutD3D9** layoutCache, GeometryD3D9** geometryCache)
{
	IDirect3DDevice9* device9 = static_cast<DeviceD3D9*>(device)->getDevice9();

	if (*geometryCache != this)
	{
		*geometryCache = this;

        if (*layoutCache != layout.get())
        {
            *layoutCache = static_cast<VertexLayoutD3D9*>(layout.get());

            device9->SetVertexDeclaration((*layoutCache)->getObject());
        }

        for (size_t i = 0; i < vertexBuffers.size(); ++i)
        {
            VertexBufferD3D9* vb = static_cast<VertexBufferD3D9*>(vertexBuffers[i].get());

            device9->SetStreamSource(i, vb->getObject(), 0, vb->getElementSize());
        }

        if (indexBuffer)
            device9->SetIndices(static_cast<IndexBufferD3D9*>(indexBuffer.get())->getObject());
	}

    if (indexBuffer)
		device9->DrawIndexedPrimitive(gGeometryPrimitiveD3D9[primitive], baseVertexIndex, indexRangeBegin, indexRangeEnd - indexRangeBegin, offset, getPrimitiveCount(primitive, count));
	else
		device9->DrawPrimitive(gGeometryPrimitiveD3D9[primitive], offset, getPrimitiveCount(primitive, count));
}

}
}
