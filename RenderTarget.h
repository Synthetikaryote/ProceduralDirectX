#pragma once
#include <d3d11_2.h>
class RenderTarget
{
public:
	RenderTarget();
	RenderTarget(unsigned width, unsigned height, DXGI_FORMAT format);
	~RenderTarget();

	void BeginRender();
	void EndRender();

	void BindTexture(unsigned index);
	void UnbindTexture(unsigned index);

	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	ID3D11RenderTargetView* renderTargetView = nullptr;
	ID3D11DepthStencilView* depthStencilView = nullptr;
	D3D11_VIEWPORT viewport;
};

