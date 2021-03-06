#pragma once
#include "Resource.h"
#include <vector>
#include <d3d11_2.h>
using namespace std;

//struct TargaHeader {
//	unsigned char data1[12];
//	unsigned short width;
//	unsigned short height;
//	unsigned char bpp;
//	unsigned char data2;
//};

class Texture : public Resource {
public:
	Texture();
	Texture(string& path);
	~Texture();

	ID3D11Texture2D* texture;
	DXGI_FORMAT format;
	ID3D11ShaderResourceView* shaderResourceView;
	bool isTextureCube = false;
	unsigned w, h, pitch;

	static Texture* Load(string& path);
	static Texture* LoadCube(vector<string>& paths);
};