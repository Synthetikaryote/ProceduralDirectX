#pragma once
#include "Resource.h"
#include <directxmath.h>
#include <d3d11_2.h>
#include "Texture.h"
#include "Shader.h"
using namespace DirectX;

struct VertexColorType {
	XMFLOAT3 position;
	XMFLOAT4 color;
};

struct VertexTextureType {
	XMFLOAT3 position;
	XMFLOAT2 texture;
};

struct Vertex {
	XMFLOAT4 position;
	XMFLOAT4 normal;
	XMFLOAT3 texture;
};

class Mesh : public Resource {
public:
	unsigned vertexCount = 0;
	unsigned indexCount = 0;
	Vertex* vertices = nullptr;
	unsigned long* indices = nullptr;
	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;
	Texture* diffuseTexture = nullptr;
	Shader* shader = nullptr;

	Mesh();
	~Mesh();
	void Release() override;
	static Mesh* Mesh::LoadCubeSphere(unsigned gridSize = 10);
	static Mesh* Mesh::LoadFromFile(string path);
	void CreateBuffers();
};
