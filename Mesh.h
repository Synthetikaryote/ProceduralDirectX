#pragma once
#include "Resource.h"
#include <directxmath.h>
#include <d3d11_2.h>

using namespace DirectX;

struct VertexColorType {
	XMFLOAT3 position;
	XMFLOAT4 color;
};

struct VertexTextureType {
	XMFLOAT3 position;
	XMFLOAT2 texture;
};

struct VertexShaderInput {
	XMFLOAT4 position;
	XMFLOAT4 normal;
	XMFLOAT3 texture;
};


class Mesh : public Resource {
public:
	Mesh();
	~Mesh();
	unsigned vertexCount = 0;
	unsigned indexCount = 0;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	static Mesh* Mesh::LoadCubeSphere(unsigned subdivisions = 2);
};

