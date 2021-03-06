#include "Shader.h"
#include "Uber.h"
#include <wrl.h>
#include <Shlwapi.h>
#include "Texture.h"
#include "ResourceManager.h"
using namespace Microsoft::WRL;

Shader::Shader() {
}


Shader::~Shader() {
	if (samplerState) samplerState->Release();
}

void Shader::SwitchTo() {
	// set the vertex and pixel shaders that will be used to render this mesh
	Uber::I().context->VSSetShader(vertexShader, NULL, 0);
	Uber::I().context->VSSetSamplers(0, 1, &samplerState);
	Uber::I().context->PSSetShader(pixelShader, NULL, 0);
	Uber::I().context->PSSetSamplers(0, 1, &samplerState);

	// set the constant buffers to their spots
	if (vertexShaderConstantBuffers.size() > 0)
		Uber::I().context->VSSetConstantBuffers(0, vertexShaderConstantBuffers.size(), &vertexShaderConstantBuffers[0]);
	if (pixelShaderConstantBuffers.size() > 0)
		Uber::I().context->PSSetConstantBuffers(0, pixelShaderConstantBuffers.size(), &pixelShaderConstantBuffers[0]);
	// set the vertex input layout
	Uber::I().context->IASetInputLayout(inputLayout);
}

void Shader::SetTexture(Texture* texture, unsigned slot) {
	Uber::I().context->PSSetShaderResources(slot, 1, &texture->shaderResourceView);
}

Shader* Shader::LoadShader(string vertexCompiledPath, string pixelCompiledPath, vector<D3D11_INPUT_ELEMENT_DESC> vertexInputDesc, D3D11_SAMPLER_DESC samplerDesc) {
	char keyString[512];
	sprintf_s(keyString, "Shader%s%s", vertexCompiledPath.size() > 0 ? vertexCompiledPath.c_str() : "(none)", pixelCompiledPath.size() > 0 ? pixelCompiledPath.c_str() : "(none)");
	size_t key = hash<string>()(string(keyString));
	return Uber::I().resourceManager->Load<Shader>(key, [&vertexCompiledPath, &pixelCompiledPath, &vertexInputDesc, &samplerDesc] {
		Shader* s = new Shader();
		ComPtr<ID3D11Device>& device = Uber::I().device;

		if (vertexCompiledPath.size() > 0) {
			auto vsBytecode = Read(vertexCompiledPath);
			ThrowIfFailed(Uber::I().device->CreateVertexShader(&vsBytecode[0], vsBytecode.size(), nullptr, &s->vertexShader));
			// this needs to match the vertex shader's input data structure
			ThrowIfFailed(device->CreateInputLayout(&vertexInputDesc[0], vertexInputDesc.size(), &vsBytecode[0], vsBytecode.size(), &s->inputLayout));
		}

		if (pixelCompiledPath.size() > 0) {
			auto psBytecode = Read(pixelCompiledPath);
			ThrowIfFailed(device->CreatePixelShader(&psBytecode[0], psBytecode.size(), nullptr, &s->pixelShader));
		}

		ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &s->samplerState));

		return s;
	});
}