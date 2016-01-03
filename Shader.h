#pragma once
#include <string>
#include <d3d11_2.h>
#include <vector>

using namespace std;

class Shader
{
public:
	Shader(string vertexCompiledPath, string pixelCompiledPath, vector<D3D11_INPUT_ELEMENT_DESC> vertexInputDesc);
	~Shader();

	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* inputLayout;
};

