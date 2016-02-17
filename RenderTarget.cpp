#include "RenderTarget.h"
#include "Uber.h"

RenderTarget::RenderTarget() {
}

RenderTarget::RenderTarget(unsigned width, unsigned height, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat, unsigned bindFlags) {
	// create the depth stencil buffer
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = colorFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	ID3D11Texture2D* texture = nullptr;
	ThrowIfFailed(Uber::I().device->CreateTexture2D(&desc, NULL, &texture));

	if (bindFlags & D3D11_BIND_DEPTH_STENCIL) {
		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Format = depthFormat;
		resourceDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = 1;
		ThrowIfFailed(Uber::I().device->CreateShaderResourceView(texture, &resourceDesc, &shaderResourceView));
	}
	else {
		ThrowIfFailed(Uber::I().device->CreateShaderResourceView(texture, nullptr, &shaderResourceView));
		ThrowIfFailed(Uber::I().device->CreateRenderTargetView(texture, nullptr, &renderTargetView));
		texture->Release();

		desc.Format = depthFormat;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ThrowIfFailed(Uber::I().device->CreateTexture2D(&desc, nullptr, &texture));
	}
	ThrowIfFailed(Uber::I().device->CreateDepthStencilView(texture, nullptr, &depthStencilView));
	texture->Release();

	viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
}


RenderTarget::~RenderTarget()
{
	if (shaderResourceView) shaderResourceView->Release();
	if (renderTargetView) renderTargetView->Release();
	if (depthStencilView) depthStencilView->Release();
}

void RenderTarget::BeginRender() {
	if (renderTargetView) {
		float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // RGBA
		Uber::I().context->ClearRenderTargetView(renderTargetView, clearColor);
	}
	Uber::I().context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	Uber::I().context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	Uber::I().context->RSSetViewports(1, &viewport);
}

void RenderTarget::EndRender() {
	// reset render target
	Uber::I().context->OMSetRenderTargets(1, &Uber::I().renderTargetView, Uber::I().depthStencilView);
	// reset viewport
	Uber::I().context->RSSetViewports(1, &Uber::I().viewport);
}

void RenderTarget::BindTexture(unsigned index) {
	Uber::I().context->PSSetShaderResources(index, 1, &shaderResourceView);
}

void RenderTarget::UnbindTexture(unsigned index) {
	static ID3D11ShaderResourceView* dummy = nullptr;
	Uber::I().context->PSSetShaderResources(index, 1, &dummy);
}