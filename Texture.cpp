#include "Texture.h"
#include "Resource.h"
#include <Shlwapi.h>
#include <string>
#include "Uber.h"
#include <functional>
#include "ResourceManager.h"
#include "FreeImage.h"
using namespace std;

void GetTextureData(const string& path, unsigned char*& data, unsigned& w, unsigned& h, DXGI_FORMAT& format, unsigned& pitch);
ID3D11Texture2D* CreateTexture2D(string path, DXGI_FORMAT& format);
ID3D11Texture2D* CreateTextureCube(vector<string> paths, DXGI_FORMAT& format);

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
		t->texture = CreateTexture2D(path, t->format);
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = t->format;
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
		t->texture = CreateTextureCube(paths, t->format);
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = t->format;
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
void GetTextureData(const string& path, unsigned char*& data, unsigned& w, unsigned& h, DXGI_FORMAT& format, unsigned& pitch) {
	// detect the format
	FREE_IMAGE_FORMAT imageFormat = FreeImage_GetFileType(path.c_str(), 0);
	// load the file
	FIBITMAP* imageData = FreeImage_Load(imageFormat, path.c_str());
	unsigned bpp = FreeImage_GetBPP(imageData);
	// keep 8 bit images as 8 bit, and tell directx the format
	switch (bpp) {
		case 8:
			format = DXGI_FORMAT_R8_UNORM;
			break;
		default:
			format = DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	// targas need to be flipped upside down
	if (imageFormat == FIF_TARGA || imageFormat == FIF_JPEG) {
		FreeImage_FlipVertical(imageData);
	}
	// make sure it's in the right byte order and size
	if (bpp > 8 && bpp != 32) {
		FIBITMAP* temp = imageData;
		imageData = FreeImage_ConvertTo32Bits(imageData);
		FreeImage_Unload(temp);
		bpp = 32;
	}
	// copy it directly to data
	w = FreeImage_GetWidth(imageData);
	h = FreeImage_GetHeight(imageData);
	pitch = w * (bpp / 8);
	unsigned long textureSize = h * pitch;
	data = new unsigned char[textureSize];
	unsigned char* bitmap = FreeImage_GetBits(imageData);
	memcpy_s(data, textureSize, bitmap, textureSize);
	FreeImage_Unload(imageData);
}

ID3D11Texture2D* CreateTexture2D(string path, DXGI_FORMAT& format) {
	unsigned char* textureData = nullptr;
	unsigned w, h, pitch;
	GetTextureData(path, textureData, w, h, format, pitch);
	DXGI_SAMPLE_DESC sampleDesc = {1, 0};
	unsigned mips = (unsigned)max(ceil(log2(w)), ceil(log2(h)));
	D3D11_TEXTURE2D_DESC textureDesc = {w, h, mips, 1, format, sampleDesc, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, D3D11_RESOURCE_MISC_GENERATE_MIPS};
	auto subresources = new D3D11_SUBRESOURCE_DATA[textureDesc.MipLevels];
	subresources[0].pSysMem = textureData;
	subresources[0].SysMemPitch = pitch;
	subresources[0].SysMemSlicePitch = h * pitch;
	ID3D11Texture2D* tex;
	ThrowIfFailed(Uber::I().device->CreateTexture2D(&textureDesc, NULL, &tex));
	Uber::I().context->UpdateSubresource(tex, D3D11CalcSubresource(0, 0, textureDesc.MipLevels), NULL, textureData, pitch, h * pitch);
	delete[] textureData;
	return tex;
}

ID3D11Texture2D* CreateTextureCube(vector<string> paths, DXGI_FORMAT& format) {
	D3D11_SUBRESOURCE_DATA subresource;
	DXGI_SAMPLE_DESC sampleDesc = {1, 0};
	D3D11_TEXTURE2D_DESC textureDesc = {0, 0, 0, 6, DXGI_FORMAT_B8G8R8A8_UNORM, sampleDesc, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 0, D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE};
	ID3D11Texture2D* tex = nullptr;
	unsigned char* textureData = nullptr;
	for (int i = 0; i < 6; ++i) {
		unsigned w, h, pitch;
		GetTextureData(paths[i], textureData, w, h, format, pitch);
		if (i == 0) {
			textureDesc.Width = w; textureDesc.Height = h;
			textureDesc.MipLevels = (unsigned)max(ceil(log2(w)), ceil(log2(h)));
			textureDesc.Format = format;
			ThrowIfFailed(Uber::I().device->CreateTexture2D(&textureDesc, NULL, &tex));
		}
		subresource.pSysMem = textureData;
		subresource.SysMemPitch = pitch;
		Uber::I().context->UpdateSubresource(tex, D3D11CalcSubresource(0, i, textureDesc.MipLevels), NULL, textureData, w * 4, w * h * 4);
	}
	delete[] textureData;
	return tex;
}
