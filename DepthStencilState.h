#pragma once
struct ID3D11DepthStencilState;
class DepthStencilState {
public:
	ID3D11DepthStencilState* depthStencilState;
	DepthStencilState(bool readable, bool writable);
	~DepthStencilState();
};

