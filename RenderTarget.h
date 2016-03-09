#pragma once
#include <d3d11_2.h>
class RenderTarget
{
public:
	RenderTarget();
	RenderTarget(unsigned width, unsigned height, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat, unsigned bindFlags);
	~RenderTarget();

	void BeginRender();
	void EndRender();

	void BindTexture(unsigned index);
	void UnbindTexture(unsigned index);
	unsigned width, height;
	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	ID3D11RenderTargetView* renderTargetView = nullptr;
	ID3D11DepthStencilView* depthStencilView = nullptr;
	D3D11_VIEWPORT viewport;
};

