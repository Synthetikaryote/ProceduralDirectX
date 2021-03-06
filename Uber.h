#pragma once

#include <d3d11_2.h>
#include <directxmath.h>
#include <comdef.h>
#include <chrono>
#include <vector>
#include <wrl.h>
#include <dinput.h>
class ResourceManager;
class Camera;
class DepthStencilState;
class HeightTree;
using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;

// constants
const float PI = 3.14159265358979f;
const float TWOPI = 2.f * PI;
const float HALFPI = 0.5f * PI;
XMFLOAT3 const worldUp = {0.0f, 1.0f, 0.0f};
XMFLOAT3 const worldForward = {0.0f, 0.0f, 1.0f};
const unsigned heightSMax = pow(2, 10);


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
float GetHeight(float yaw, float pitch);
void SetHeight(float yaw, float pitch, float value);

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

	HeightTree* heights;
	unsigned zoomStep = 1;
	HWND hWnd;
	WNDCLASSEX wc;
	ComPtr<ID3D11Device> device;
	ID3D11DeviceContext* context;
	ResourceManager* resourceManager;
	unsigned char keyboardState[256];
	DIMOUSESTATE mouseState;
	int mouseX = 0, mouseY = 0;
	Camera* camera;
	Camera* lightCamera;
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
	DepthStencilState* depthStencilState;
};