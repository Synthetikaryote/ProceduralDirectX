#include "Texture.h"
#include "Resource.h"
#include <Shlwapi.h>
#include <string>
#include "Uber.h"
#include <functional>
using namespace std;

void GetTextureData(const string& path, unsigned char*& data, unsigned& w, unsigned& h);
ID3D11Texture2D* CreateTexture2D(string path);
ID3D11Texture2D* CreateTextureCube(vector<string> paths);

Texture::Texture() {}
Texture::~Texture() {
	if (shaderResourceView) shaderResourceView->Release();
	if (texture) texture->Release();
}

Texture* Texture::Load(string& path) {
	char* filename = PathFindFileNameA(path.c_str());
	size_t key = hash<string>()(string(filename));
	return Uber::I().resourceManager->Load<Texture>(key, [path]{
		Texture* t = new Texture();
		t->texture = CreateTexture2D(path);
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		ThrowIfFailed(Uber::I().device->CreateShaderResourceView(t->texture, &srvDesc, &t->shaderResourceView));
		Uber::I().context->GenerateMips(t->shaderResourceView);
		return t;
	});
}

Texture* Texture::LoadCube(vector<string>& paths) {
	assert(paths.size() == 6);
	size_t key = 0;
	for (auto& path : paths) {
		char* filename = PathFindFileNameA(path.c_str());
		size_t keyPart = hash<string>()(string(filename));
		if (!key)
			key = keyPart;
		else
			hash_combine(key, keyPart);
	}
	return Uber::I().resourceManager->Load<Texture>(key, [paths] {
		Texture* t = new Texture();
		t->texture = CreateTextureCube(paths);
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		ThrowIfFailed(Uber::I().device->CreateShaderResourceView(t->texture, &srvDesc, &t->shaderResourceView));
		Uber::I().context->GenerateMips(t->shaderResourceView);
		t->isTextureCube = true;
		return t;
	});
}


// private functions

// this assumes it's a targa for now
void GetTextureData(const string& path, unsigned char*& data, unsigned& w, unsigned& h) {
	FILE* filePtr;
	TargaHeader targaHeader;
	assert(fopen_s(&filePtr, path.c_str(), "rb") == 0);
	assert((unsigned)fread(&targaHeader, sizeof(TargaHeader), 1, filePtr) == 1);
	assert(targaHeader.bpp == 32 || targaHeader.bpp == 24);
	unsigned bytespp = targaHeader.bpp / 8;
	w = targaHeader.width; h = targaHeader.height;
	unsigned imageSize = w * h * bytespp;
	auto imageData = new unsigned char[imageSize];
	assert(imageData);
	unsigned textureSize = targaHeader.width * targaHeader.height * 4;
	data = new unsigned char[textureSize];
	assert(data);
	assert((unsigned)fread(imageData, 1, imageSize, filePtr) == imageSize);
	assert(fclose(filePtr) == 0);
	unsigned n = 0;
	// targa stores it upside down, so go through the rows backwards
	for (int r = (int)targaHeader.height - 1; r >= 0; --r) { // signed because it must become -1
		for (unsigned j = r * targaHeader.width * bytespp; j < (r + 1) * targaHeader.width * bytespp; j += bytespp) {
			data[n++] = imageData[j + 2]; // red
			data[n++] = imageData[j + 1]; // green
			data[n++] = imageData[j + 0]; // blue
			data[n++] = bytespp == 4 ? imageData[j + 3] : 255; // alpha
		}
	}
	delete[] imageData;
}

ID3D11Texture2D* CreateTexture2D(string path) {
	unsigned char* textureData = nullptr;
	unsigned w, h;
	GetTextureData(path, textureData, w, h);
	DXGI_SAMPLE_DESC sampleDesc = {1, 0};
	unsigned mips = (unsigned)max(ceil(log2(w)), ceil(log2(h)));
	D3D11_TEXTURE2D_DESC textureDesc = {w, h, mips, 1, DXGI_FORMAT_R8G8B8A8_UNORM, sampleDesc, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, D3D11_RESOURCE_MISC_GENERATE_MIPS};
	auto subresources = new D3D11_SUBRESOURCE_DATA[textureDesc.MipLevels];
	subresources[0].pSysMem = textureData;
	subresources[0].SysMemPitch = w * 4;
	subresources[0].SysMemSlicePitch = w * h * 4;
	ID3D11Texture2D* tex;
	ComPtr<ID3D11Device>& device = Uber::I().device;
	ThrowIfFailed(device->CreateTexture2D(&textureDesc, NULL, &tex));
	Uber::I().context->UpdateSubresource(tex, D3D11CalcSubresource(0, 0, textureDesc.MipLevels), NULL, textureData, w * 4, w * h * 4);
	delete[] textureData;
	return tex;
}

ID3D11Texture2D* CreateTextureCube(vector<string> paths) {
	D3D11_SUBRESOURCE_DATA subresource;
	DXGI_SAMPLE_DESC sampleDesc = {1, 0};
	D3D11_TEXTURE2D_DESC textureDesc = {0, 0, 0, 6, DXGI_FORMAT_R8G8B8A8_UNORM, sampleDesc, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE};
	ID3D11Texture2D* tex = nullptr;
	unsigned char* textureData = nullptr;
	for (int i = 0; i < 6; ++i) {
		unsigned w, h;
		GetTextureData(paths[i], textureData, w, h);
		if (i == 0) {
			textureDesc.Width = w; textureDesc.Height = h;
			textureDesc.MipLevels = (unsigned)max(ceil(log2(w)), ceil(log2(h)));
			ThrowIfFailed(Uber::I().device->CreateTexture2D(&textureDesc, NULL, &tex));
		}
		subresource.pSysMem = textureData;
		subresource.SysMemPitch = w * 4;
		Uber::I().context->UpdateSubresource(tex, D3D11CalcSubresource(0, i, textureDesc.MipLevels), NULL, textureData, w * 4, w * h * 4);
	}
	delete[] textureData;
	return tex;
}
