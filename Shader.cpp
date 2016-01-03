#include "Shader.h"
#include "Uber.h"
#include <wrl.h>
using namespace Microsoft::WRL;

Shader::Shader(string vertexCompiledPath, string pixelCompiledPath, vector<D3D11_INPUT_ELEMENT_DESC> vertexInputDesc)
{
	auto vsBytecode = Read(vertexCompiledPath);
	ComPtr<ID3D11Device>& device = Uber::I().device;
	ThrowIfFailed(Uber::I().device->CreateVertexShader(&vsBytecode[0], vsBytecode.size(), nullptr, &vertexShader));

	auto psBytecode = Read(pixelCompiledPath);
	ThrowIfFailed(device->CreatePixelShader(&psBytecode[0], psBytecode.size(), nullptr, &pixelShader));

	// this needs to match the vertex shader's input data structure
	ThrowIfFailed(device->CreateInputLayout(&vertexInputDesc[0], vertexInputDesc.size(), &vsBytecode[0], vsBytecode.size(), &inputLayout));
}


Shader::~Shader()
{
}
