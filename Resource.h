#pragma once
#include <string>
using namespace std;
class Resource
{
public:
	Resource();
	~Resource();
	void AddRef();
	virtual void Release();
	unsigned refCount;
};

