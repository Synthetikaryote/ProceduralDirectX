#pragma once
#include "Resource.h"
#include <vector>
#include <Shlwapi.h>
#include "Uber.h"
using namespace std;

class Texture : public Resource {
public:
	Texture();
	Texture(string path);
	~Texture();

	static Texture* Load(string path);
	static Texture* LoadTextureCube(vector<string> paths);
};