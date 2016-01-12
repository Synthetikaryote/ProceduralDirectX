#pragma once
#include <string>
using namespace std;
class Resource
{
public:
	Resource();
	~Resource();
	void AddRef();
	void Release();
	unsigned refCount;
};

