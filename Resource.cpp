#include "Resource.h"
#include <cassert>


Resource::Resource()
{
}


Resource::~Resource()
{
}

void Resource::AddRef() {
	++refCount;
}
void Resource::Release() {
	assert(refCount > 0);
	--refCount;
	if (refCount == 0) {
		delete this;
	}
}