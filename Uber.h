#pragma once

#include <d3d11_2.h>
#include <comdef.h>
#include <chrono>
#include <vector>
#include <wrl.h>
#include <dinput.h>
class ResourceManager;
class Camera;
using namespace std;
using namespace Microsoft::WRL;

// constants
const float PI = 3.14159265358979f;
const float TWOPI = 2.f * PI;
const float HALFPI = 0.5f * PI;

// global functions

// convert DirectX error codes to exceptions
void ThrowIfFailed(HRESULT hr);

// C++11 timestamp as a float
float time();

// Read a file in binary and return a vector of bytes
vector<uint8_t> Read(string path);

// Hash combiner
template <class T>
inline void hash_combine(size_t& seed, const T& v) {
	seed ^= hash<T>()(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
}

bool IsKeyDown(unsigned char);

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

	HWND hWnd;
	WNDCLASSEX wc;
	ComPtr<ID3D11Device> device;
	ID3D11DeviceContext* context;
	ResourceManager* resourceManager;
	unsigned char keyboardState[256];
	DIMOUSESTATE mouseState;
	int mouseX = 0, mouseY = 0;
	Camera* camera;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;
	D3D11_VIEWPORT viewport;
	bool cursorVisible = true;
	POINT savedMousePos;
	bool windowIsFocused = true;
	unsigned screenWidth = 0;
	unsigned screenHeight = 0;
	unsigned windowWidth = 0;
	unsigned windowHeight = 0;
	unsigned windowLeft = 0;
	unsigned windowTop = 0;
};