#include "Texture.h"
#include "Resource.h"
#include <vector>
#include <Shlwapi.h>
#include "Uber.h"
using namespace std;

Texture::Texture() {}
Texture::Texture(string path) {}
Texture::~Texture() {}

static Texture* Load(string path) {
	char* filename = PathFindFileNameA(path.c_str());
	size_t key = hash<string>()(string(filename));

	auto& rm = Uber::I().resourceManager;
	Texture* t = rm->GetAsset<Texture>(key);
	if (!t) {
		char* extension = PathFindExtensionA(filename);
		assert(extension);
		if (strcmp(extension, ".tga")) {
			t = &Texture(path);
		}
		assert(t);
		rm->SaveAsset<Texture>(key, t);
	}
	return t;

}

static Texture* LoadTextureCube(vector<string> paths) {
}