#pragma once
#include "Resource.h"
#include <directxmath.h>
#include <d3d11_2.h>
#include <vector>
#include "Uber.h"
class Texture;
class Shader;
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
	XMFLOAT4 tangent;
	XMFLOAT3 texture;
};

enum ShaderType {
	ShaderTypePixel = 0,
	ShaderTypeVertex = 1
};

struct TextureBinding {
	Texture* texture;
	ShaderType shaderType;
	unsigned shaderSlot;
};

class Mesh : public Resource {
public:
	unsigned vertexCount = 0;
	unsigned indexCount = 0;
	Vertex* vertices = nullptr;
	unsigned long* indices = nullptr;
	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11Buffer* indexBuffer = nullptr;
	vector<TextureBinding> textureBindings;
	Shader* shader = nullptr;
	vector<TextureBinding> depthMapTextureBindings;

	Mesh();
	~Mesh();
	void Release() override;
	void Draw();
	void DrawDepthMap();
	void DrawWithBindings(vector<TextureBinding>& bindings);
	void UpdatePartialSphere(unsigned longitudes = 24, unsigned latitudes = 24, float yawMin = 0.0f, float yawMax = TWOPI, float pitchMin = 0.0f, float pitchMax = PI);

	static Mesh* LoadQuad();
	static Mesh* LoadPlane(unsigned columns = 1, unsigned rows = 1);
	static Mesh* LoadCubeSphere(unsigned gridSize = 10);
	static Mesh* LoadSphere(unsigned longitudes = 24, unsigned latitudes = 24);
	static Mesh* LoadPartialSphere(unsigned longitudes = 24, unsigned latitudes = 24, float yawMin = 0.0f, float yawMax = TWOPI, float pitchMin = 0.0f, float pitchMax = PI, unsigned flags = 0u);
	static Mesh* LoadRecursiveHemisphere(unsigned gridSize = 4, unsigned iterations = 3);
	static Mesh* LoadFromFile(string path);
	void CreateBuffers(unsigned flags = 0u);
};
