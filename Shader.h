#pragma once
#include "Resource.h"
#include <string>
#include <d3d11_2.h>
#include <vector>
class Texture;

enum PSFlags {
	CelShading = 1
};

using namespace std;

class Shader : public Resource {
public:
	Shader();
	~Shader();

	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* inputLayout;
	ID3D11SamplerState* samplerState;

	vector<ID3D11Buffer*> vertexShaderConstantBuffers;
	vector<ID3D11Buffer*> pixelShaderConstantBuffers;

	void SwitchTo();
	void SetTexture(Texture* texture, unsigned slot);

	static Shader* LoadShader(string vertexCompiledPath, string pixelCompiledPath, vector<D3D11_INPUT_ELEMENT_DESC> vertexInputDesc, D3D11_SAMPLER_DESC samplerDesc);
};

