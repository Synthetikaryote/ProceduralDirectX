#pragma once
#include <d3d11_2.h>
#include <directxmath.h>
#include "Uber.h"
using namespace DirectX;

struct MatrixBufferType {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};
struct MaterialBufferType {
	XMFLOAT4 materialAmbient;
	XMFLOAT4 materialDiffuse;
	XMFLOAT4 materialSpecular;
	unsigned vsSlotsUsed;
	unsigned psSlotsUsed;
};
struct LightingBufferType {
	XMFLOAT4 viewPosition;
	XMFLOAT4 lightDirection;
	XMFLOAT4 lightAmbient;
	XMFLOAT4 lightDiffuse;
	XMFLOAT4 lightSpecular;
};
struct BrushBufferType {
	XMFLOAT4 cursorPosition;
	unsigned cursorFlags;
	float cursorRadiusSq;
	float cursorLineThickness;
};

template <typename T>
class ConstantBuffer {
public:
	T data;
	ID3D11Buffer* buffer = nullptr;

	ConstantBuffer() {
		D3D11_BUFFER_DESC desc;
		desc.Usage = D3D11_USAGE_DEFAULT;
		const unsigned typeSize = sizeof(T);
		// must be a multiple of 16 bytes
		desc.ByteWidth = (typeSize % 16) ? ((typeSize / 16) + 1) * 16 : typeSize;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		ThrowIfFailed(Uber::I().device->CreateBuffer(&desc, NULL, &buffer));
		ZeroMemory(&data, sizeof T);
	}
	~ConstantBuffer() {
		if (buffer) buffer->Release();
	}
	void UpdateSubresource() {
		Uber::I().context->UpdateSubresource(buffer, 0, nullptr, static_cast<const void*>(&data), 0, 0);
	}
};

