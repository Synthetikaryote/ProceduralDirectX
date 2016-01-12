#include "ResourceManager.h"
#include <Shlwapi.h>
#include <cassert>
#include "Texture.h"
#include <functional>
#include <string>

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

Resource* ResourceManager::Load(string path) {
	char* filename = PathFindFileNameA(path.c_str());
	char* extension = PathFindExtensionA(filename);
	
	size_t key = hash<string>()(string(filename));
	Resource* r = assets[key];
	if (!r) {
		assert(extension);
		if (strcmp(extension, ".tga")) {
			Texture t = Texture(path);
			r = &t;
		}
		else {
			// the manager doesn't know how to load this type of file
		}
		assert(r);
		assets[key] = r;
		r->AddRef(); // manager's reference
	}
	r->AddRef();
	return r;
}

void ResourceManager::Terminate() {
	for (auto kv : assets) {
		assert(kv.second->refCount == 1);
		delete kv.second;
	}
}
