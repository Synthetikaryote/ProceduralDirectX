#pragma once
#include "Resource.h"
#include <map>
using namespace std;
class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();
	Resource* Load(string path);
	void Terminate();
private:
	map<unsigned, Resource*> assets;
};

