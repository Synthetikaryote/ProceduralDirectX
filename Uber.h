#pragma once

#include <d3d11_2.h>
#include <comdef.h>
#include <chrono>
#include <vector>
#include <wrl.h>

using namespace std;
using namespace Microsoft::WRL;

// global functions

// convert DirectX error codes to exceptions
void ThrowIfFailed(HRESULT hr);

// C++11 timestamp as a float
float time();

// Read a file in binary and return a vector of bytes
vector<uint8_t> Read(string path);

// singleton pattern from http://stackoverflow.com/questions/1008019/c-singleton-design-pattern
class Uber {
public:
	static Uber& I() {
		static Uber instance; // Guaranteed to be destroyed.
		// Instantiated on first use.
		return instance;
	}
private:
	Uber() {}; // Constructor? (the {} braces) are needed here.

public:
	Uber(Uber const&) = delete;
	void operator=(Uber const&) = delete;

	ComPtr<ID3D11Device> device;
};