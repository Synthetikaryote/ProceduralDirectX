#pragma once
#include "Resource.h"
#include <map>
#include <functional>
using namespace std;

class ResourceManager {
private:
	map<unsigned, Resource*> assets;

public:
	ResourceManager();
	~ResourceManager();

	template <class T>
	T* GetAsset(unsigned key);
	template <class T>
	void SaveAsset(unsigned key, T* res);
	template <class T>
	T* Load(unsigned key, function<T*()> create);

	void Terminate();
};

template <class T>
T* ResourceManager::GetAsset(unsigned key) {
	static_assert(is_base_of<Resource, T>::value, "T must derive from Resource");
	Resource* r = assets[key];
	if (!r)
		return nullptr;
	r->AddRef();
	T* t = static_cast<T*>(r);
	return t;
}

template <class T>
void ResourceManager::SaveAsset(unsigned key, T* res) {
	assert(res);
	assert(!GetAsset<T>(key));
	static_assert(is_base_of<Resource, T>::value, "T must derive from Resource");
	assets[key] = res;
	res->AddRef(); // resource manager's reference
	res->AddRef(); // user's reference
}

template <class T>
T* ResourceManager::Load(unsigned key, function<T*()> create) {
	T* t = GetAsset<T>(key);
	if (!t) {
		t = create();
		assert(t);
		SaveAsset<T>(key, t);
	}
	return t;
}
