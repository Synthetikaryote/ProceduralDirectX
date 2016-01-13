#include "ResourceManager.h"
#include "Texture.h"
#include <vector>
#include <cassert>
#include <functional>
#include <string>
using namespace std;
ResourceManager::ResourceManager() {}
ResourceManager::~ResourceManager() {}


void ResourceManager::Terminate() {
	for (auto kv : assets) {
		assert(kv.second->refCount == 1);
		kv.second->Release();
	}
}
