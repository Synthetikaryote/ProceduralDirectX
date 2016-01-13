// include the basic windows header file
#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#include <d3d11_2.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <sstream>
#include <tuple>
#include <vector>
#include <map>
#include <wrl.h>
#include <string>

#include "Uber.h"
#include "Shader.h"
#include "Texture.h"

// text rendering
#include <d2d1_2.h>
#include <dwrite.h>

using namespace DirectX;
using namespace std;
using namespace Microsoft::WRL;

struct MatrixBufferType {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};
struct MaterialBufferType {
	XMFLOAT4 materialAmbient;
	XMFLOAT4 materialDiffuse;
	XMFLOAT4 materialSpecular;
};
struct LightingBufferType {
	XMFLOAT4 viewPosition;
	XMFLOAT4 lightDirection;
	XMFLOAT4 lightAmbient;
	XMFLOAT4 lightDiffuse;
	XMFLOAT4 lightSpecular;
};

struct VertexColorType {
	XMFLOAT3 position;
	XMFLOAT4 color;
};

struct VertexTextureType {
	XMFLOAT3 position;
	XMFLOAT2 texture;
};

struct VertexShaderInput {
	XMFLOAT4 position;
	XMFLOAT4 normal;
	XMFLOAT3 texture;
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// the handle for the window, filled by a function
	HWND hWnd;
	// this struct holds information for the window class
	WNDCLASSEX wc;

	// clear out the window class for use
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";

	// register the window class
	RegisterClassEx(&wc);

	bool vsync = false;
	bool windowed = true;

	int screenWidth = windowed ? 1000 : GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = windowed ? 800 : GetSystemMetrics(SM_CYSCREEN);
	RECT wr = { 0, 0, screenWidth, screenHeight };    // set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

	// create the window and use the result as the handle
	hWnd = CreateWindowEx(NULL,
		L"WindowClass1",    // name of the window class
		L"Our First Windowed Program",   // title of the window
		WS_OVERLAPPEDWINDOW,    // window style
		300,    // x-position of the window
		50,    // y-position of the window
		wr.right - wr.left,    // width of the window
		wr.bottom - wr.top,    // height of the window
		NULL,    // we have no parent window, NULL
		NULL,    // we aren't using menus, NULL
		hInstance,    // application handle
		NULL);    // used with multiple windows, NULL

	// display the window on the screen
	ShowWindow(hWnd, nCmdShow);

	IDXGIFactory* factory;
	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	IDXGIAdapter* adapter;
	factory->EnumAdapters(0, &adapter);
	IDXGIOutput* adapterOutput;
	adapter->EnumOutputs(0, &adapterOutput);

	// DirectX init
	// device, context, and rendertargetview
	ID3D11DeviceContext* context = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};
	unsigned flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	ComPtr<ID3D11Device>& device = Uber::I().device;
	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, flags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &device, NULL, &context);
	Uber::I().context = context;

	// create a direct2d device and context
	D2D1_FACTORY_OPTIONS d2dOptions;
	ZeroMemory(&d2dOptions, sizeof(D2D1_FACTORY_OPTIONS));
#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers.
	d2dOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	ID2D1Factory2* d2dFactory;
	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory2), &d2dOptions, reinterpret_cast<void**>(&d2dFactory)));
	ID2D1Device1* d2dDevice;
	// get the underlying DXGI device of the d3d device
	ComPtr<ID3D11Device2> d3dDevice;
	ThrowIfFailed(device.As(&d3dDevice));
	ComPtr<IDXGIDevice> dxgiDevice;
	ThrowIfFailed(d3dDevice.As(&dxgiDevice));
	ThrowIfFailed(d2dFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice));
	ID2D1DeviceContext* d2dContext;
	ThrowIfFailed(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext));

	// initialize the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2; // double buffering to reduce latency
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	// turn multisampling off
	swapChainDesc.SampleDesc.Count = 1;
	unsigned qualityLevels;
	device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 2, &qualityLevels);
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = windowed;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// don't set the advanced flags
	swapChainDesc.Flags = 0;
	// set the refresh rate
	if (vsync) {
		// get the refresh rate from the graphics card
		unsigned numModes;
		adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
		DXGI_MODE_DESC* displayModeList = new DXGI_MODE_DESC[numModes];
		adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
		for (unsigned i = 0; i < numModes; i++) {
			if (displayModeList[i].Width == (unsigned)screenWidth && displayModeList[i].Height == (unsigned)screenHeight) {
				swapChainDesc.BufferDesc.RefreshRate.Numerator = displayModeList[i].RefreshRate.Numerator;
				swapChainDesc.BufferDesc.RefreshRate.Denominator = displayModeList[i].RefreshRate.Denominator;
				break;
			}
		}
		delete[] displayModeList;
	}
	else {
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	//// find the video memory in megabytes
	//DXGI_ADAPTER_DESC adapterDesc;
	//adapter->GetDesc(&adapterDesc);
	//size_t videoMemoryMb = adapterDesc.DedicatedVideoMemory / 1024 / 1024;

	//// get the video card name
	//char videoCardName[128];
	//size_t stringLength;
	//wcstombs_s(&stringLength, videoCardName, 128, adapterDesc.Description, 128);

	IDXGISwapChain* swapChain = nullptr;
	factory->CreateSwapChain(device.Get(), &swapChainDesc, &swapChain);
	ID3D11Texture2D* backBuffer = 0;
	swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	ID3D11RenderTargetView* renderTargetView;
	device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);

	adapterOutput->Release();
	adapter->Release();
	factory->Release();
	backBuffer->Release();

	// create the depth stencil buffer
	ID3D11Texture2D* depthStencilBuffer = nullptr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
	depthBufferDesc.Width = screenWidth;
	depthBufferDesc.Height = screenHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;
	device->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer);

	// create the depth stencil state
	ID3D11DepthStencilState* depthStencilState = nullptr;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
	context->OMSetDepthStencilState(depthStencilState, 1);  // make it take effect

	// create the depth stencil view
	ID3D11DepthStencilView* depthStencilView = nullptr;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);

	// custom raster options, like draw as wireframe
	ID3D11RasterizerState* rasterState = nullptr;
	D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	device->CreateRasterizerState(&rasterDesc, &rasterState);
	context->RSSetState(rasterState);

	// set up the viewport
	D3D11_VIEWPORT viewport;
	viewport.Width = (float)screenWidth;
	viewport.Height = (float)screenHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	context->RSSetViewports(1, &viewport);

	// create a Direct2D target bitmap associated with the swap chain back buffer and set it as the current target
	float dpiX = 300.0f;
	float dpiY = 300.0f;
	d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
	D2D1_BITMAP_PROPERTIES1 bitmapProperties;
	bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	bitmapProperties.dpiX = dpiX;
	bitmapProperties.dpiY = dpiY;
	bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	bitmapProperties.colorContext = NULL;
	ComPtr<IDXGISurface> dxgiBackBuffer;
	ThrowIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));
	ComPtr<ID2D1Bitmap1> d2dTargetBitmap = nullptr;
	ThrowIfFailed(d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bitmapProperties, &d2dTargetBitmap));
	d2dContext->SetTarget(d2dTargetBitmap.Get());
	d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	d2dContext->SetDpi(dpiX, dpiY);

	// create the projection matrix
	const float PI = 3.14159265358979f;
	float fieldOfView = PI / 4.0f;
	float screenAspect = (float)screenWidth / (float)screenHeight;
	float screenDepth = 1000.0f;
	float screenNear = 0.1f;
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	// create the world matrix
	XMMATRIX worldMatrix = XMMatrixIdentity();

	// create an orthographic projection matrix for 2D UI rendering.
	XMMATRIX orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	// create the resource manager
	Uber::I().resourceManager = new ResourceManager();

	// shaders
	Shader litTexture("LitTextureVS.cso", "LitTexturePS.cso", {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	});

	// create a texture sampler state
	D3D11_SAMPLER_DESC samplerDesc = {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0.0f, 1, D3D11_COMPARISON_ALWAYS, {0, 0, 0, 0}, 0, D3D11_FLOAT32_MAX};
	ID3D11SamplerState* samplerState;
	ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &samplerState));

	// dynamic matrix constant buffer that's in the vertex shaders
	ID3D11Buffer* matrixBuffer = 0;
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	ThrowIfFailed(device->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer));

	// dynamic material constant buffer
	ID3D11Buffer* materialBuffer = 0;
	D3D11_BUFFER_DESC materialBufferDesc;
	materialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	materialBufferDesc.ByteWidth = sizeof(MaterialBufferType);
	materialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	materialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	materialBufferDesc.MiscFlags = 0;
	materialBufferDesc.StructureByteStride = 0;
	ThrowIfFailed(device->CreateBuffer(&materialBufferDesc, NULL, &materialBuffer));

	// dynamic lighting constant buffer
	ID3D11Buffer* lightingBuffer = 0;
	D3D11_BUFFER_DESC lightingBufferDesc;
	lightingBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightingBufferDesc.ByteWidth = sizeof(LightingBufferType);
	lightingBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightingBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightingBufferDesc.MiscFlags = 0;
	lightingBufferDesc.StructureByteStride = 0;
	ThrowIfFailed(device->CreateBuffer(&lightingBufferDesc, NULL, &lightingBuffer));

	// put all the buffers in an ordered array
	ID3D11Buffer* constantBuffers[] = { matrixBuffer, materialBuffer, lightingBuffer };

	//// make a triangle
	//unsigned vertexCount = 3;
	//unsigned indexCount = 3;
	//VertexTextureType* vertices = new VertexTextureType[vertexCount];
	//unsigned long* indices = new unsigned long[indexCount];
	//vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // bottom left
	//vertices[0].texture = XMFLOAT2(0.0f, 1.0f);
	////vertices[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	//vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // top middle
	//vertices[1].texture = XMFLOAT2(0.5f, 0.0f);
	////vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	//vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // bottom right
	//vertices[2].texture = XMFLOAT2(1.0f, 1.0f);
	////vertices[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	//indices[0] = 0;  // bottom left
	//indices[1] = 1;  // top middle
	//indices[2] = 2;  // bottom right

	// make a sphere
	// note: go to planetpixelemporium for textures
	//const float TWOPI = 2.f * PI;
	//unsigned latitudes = 24;
	//unsigned longitudes = 24;
	//unsigned vertexCount = (latitudes + 1) * (longitudes + 1);
	//unsigned indexCount = (latitudes - 1) * (longitudes + 1) * 2 * 3;
	//VertexShaderInput* vertices = new VertexShaderInput[vertexCount];
	//unsigned long* indices = new unsigned long[indexCount];
	//const float latStep = PI / latitudes;
	//const float lonStep = TWOPI / longitudes;
	//unsigned v = 0;
	//for (unsigned lat = 0; lat <= latitudes; ++lat) {
	//	for (unsigned lon = 0; lon <= longitudes; ++lon) {
	//		const float alat = lat * latStep;
	//		const float alon = lon * lonStep;
	//		vertices[v].position = XMFLOAT4(sin(alat) * cos(alon), cos(alat), sin(alat) * sin(alon), 1.0f);
	//		vertices[v].normal = vertices[v].position;
	//		vertices[v].normal.w = 0.0f;
	//		vertices[v++].texture = XMFLOAT2((float)lon / longitudes, -cos(alat) * 0.5f + 0.5f);
	//	}
	//}
	//unsigned index = 0;
	//for (unsigned lat = 0; lat < latitudes; ++lat) {
	//	for (unsigned lon = 0; lon < longitudes; ++lon) {
	//		if (lat != latitudes - 1) {
	//			indices[index++] = lat * (longitudes + 1) + (lon % (longitudes + 1));
	//			indices[index++] = (lat + 1) * (longitudes + 1) + ((lon + 1) % (longitudes + 1));
	//			indices[index++] = (lat + 1) * (longitudes + 1) + (lon % (longitudes + 1));
	//		}
	//		if (lat != 0) {
	//			indices[index++] = lat * (longitudes + 1) + (lon % (longitudes + 1));
	//			indices[index++] = lat * (longitudes + 1) + ((lon + 1) % (longitudes + 1));
	//			indices[index++] = (lat + 1) * (longitudes + 1) + ((lon + 1) % (longitudes + 1));
	//		}
	//	}
	//}

	// make a cube sphere
	// (quadrilateralized spherical cube)
	const float TWOPI = 2.f * PI;
	unsigned subdivisions = 19;
	// base cube's 8 vertices + the subdivisions on each edge + the subdivisions in each face + special edges for uv fixing
	unsigned vertexCount = 8 + 12 * subdivisions + 6 * subdivisions * subdivisions + 12 * (subdivisions + 2);
	// for each face, there are 3 indices per triangle, 2 triangles per quad, and (subdivisions + 1)^2 quads
	unsigned indexCount = 6 * 3 * 2 * (subdivisions + 1) * (subdivisions + 1);
	VertexShaderInput* vertices = new VertexShaderInput[vertexCount];
	struct loc {
		unsigned x, y, z, s;
		bool operator<(const loc &o) const {
			return (x < o.x || (x == o.x && (y < o.y || (y == o.y && (z < o.z || (z == o.z && s < o.s))))));
		}
	};
	map<loc, unsigned> indexRef;
	unsigned long* indices = new unsigned long[indexCount];
	unsigned v = 0;
	float yaw = 0.f;
	float pitch = 0.f;
	unsigned w = subdivisions + 1;
	const float step = 1.f / w;
	loc p;
	// vertices
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned r = 0; r < w + 1; ++r) {
			for (unsigned c = 0; c < w + 1; ++c) {
				switch (i) {
					case 0: { p = { w, r, w - c, i }; break; } // right
					case 1: { p = { 0, r, c, i }; break; } // left
					case 2: { p = { c, 0, r, i }; break; } // up
					case 3: { p = { c, w, w - r, i }; break; } // down
					case 4: { p = { c, r, w, i }; break; } // front
					case 5: { p = { w - c, r, 0, i }; break; } // back
				}
				indexRef[p] = v;
				float x = p.x * 2.f / w - 1, y = (w - p.y) * 2.f / w - 1, z = (w - p.z) * 2.f / w - 1;
				float x2 = x * x, y2 = y * y, z2 = z * z;
				vertices[v].position = XMFLOAT4(
					x * sqrt(1.f - y2 / 2.f - z2 / 2.f + y2 * z2 / 3.f),
					y * sqrt(1.f - x2 / 2.f - z2 / 2.f + x2 * z2 / 3.f),
					z * sqrt(1.f - x2 / 2.f - y2 / 2.f + x2 * y2 / 3.f),
					1.f);
				vertices[v].normal = vertices[v].position;
				vertices[v].normal.w = 0.0f;
				vertices[v].texture = XMFLOAT3(-x, y, z);
				++v;
			}
		}
	}
	// indices
	unsigned index = 0;
	auto getIndex = [indexRef, w](unsigned i, unsigned c, unsigned r) {
		unsigned x = 0, y = 0, z = 0, s = 0;
		switch (i) {
			case 0: { x = w, y = r, z = w - c, s = i; break; } // right
			case 1: { x = 0, y = r, z = c; s = i; break; } // left
			case 2: { x = c, y = 0, z = r, s = i; break;  } // up
			case 3: { x = c, y = w, z = w - r, s = i; break; } // down
			case 4: { x = c, y = r, z = w; s = i; break; } // front
			case 5: { x = w - c, y = r, z = 0, s = i; break; } // back
		}
		return indexRef.at({ x, y, z, s });
	};
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned r = 0; r < w; ++r) {
			for (unsigned c = 0; c < w; ++c) {
				indices[index++] = getIndex(i, c, r);
				indices[index++] = getIndex(i, c + 1, r);
				indices[index++] = getIndex(i, c + 1, r + 1);
				indices[index++] = indices[index - 3];
				indices[index++] = indices[index - 2];
				indices[index++] = getIndex(i, c, r + 1);
			}
		}
	}

	// create the vertex and index buffers
	ID3D11Buffer *vertexBuffer, *indexBuffer;
	for (auto bufferInfo : {
		tuple<unsigned, void*, ID3D11Buffer**, unsigned>(sizeof(VertexShaderInput) * vertexCount, vertices, &vertexBuffer, D3D11_BIND_VERTEX_BUFFER),
		tuple<unsigned, void*, ID3D11Buffer**, unsigned>(sizeof(unsigned long) * indexCount, indices, &indexBuffer, D3D11_BIND_INDEX_BUFFER)
	}) {
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = get<0>(bufferInfo);
		bufferDesc.BindFlags = get<3>(bufferInfo);
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		// give the subresource structure a pointer to the raw data
		D3D11_SUBRESOURCE_DATA bufferData;
		bufferData.pSysMem = get<1>(bufferInfo);
		bufferData.SysMemPitch = 0;
		bufferData.SysMemSlicePitch = 0;

		// create the buffer
		ThrowIfFailed(device->CreateBuffer(&bufferDesc, &bufferData, get<2>(bufferInfo)));
	}
	delete[] vertices;
	delete[] indices;


	// set up the camera
	XMFLOAT3 position(0.f, 0.f, -3.f);
	XMFLOAT3 rotation(0.f, 0.f, 0.f);
	XMFLOAT3 up(0.f, 1.f, 0.f);
	XMFLOAT3 forward(0.f, 0.f, 1.f);


	// texture
	vector<string> paths(6);
	for (int i = 0; i < 6; ++i) {
		char fileName[40];
		sprintf_s(fileName, "earth_%c.tga", "rludfb"[i]);
		paths[i] = string(fileName);
	}
	auto* worldTexture = Texture::LoadCube(paths);

	// text
	// create a white brush
	ID2D1SolidColorBrush* whiteBrush;
	ThrowIfFailed(d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush));
	// create the directWrite factory
	IDWriteFactory* writeFactory;
	ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&writeFactory)));
	IDWriteTextFormat* textFormat;
	// font name, font collection (NULL = system)
	ThrowIfFailed(writeFactory->CreateTextFormat(L"Consolas", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.0f, L"en-us", &textFormat));
	// align top-left
	ThrowIfFailed(textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));
	ThrowIfFailed(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));


	// main loop
	MSG msg = { 0 };
	unsigned framesDrawn = 0;
	float lastFrameTime = 0;
	float fpsElapsed = 1.f;
	float fpsUpdateDelay = 0.5f;
	UINT32 fpsLength = 0;
	wchar_t fps[80];
	while (true) {
		// Check to see if any messages are waiting in the queue
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			// translate keystroke messages into the right format
			TranslateMessage(&msg);

			// send the message to the WindowProc function
			DispatchMessage(&msg);

			// check to see if it's time to quit
			if (msg.message == WM_QUIT)
				break;
		}
		else {
			float elapsed = time() - lastFrameTime;
			lastFrameTime = time();

			// bind the render target view and depth stencil buffer to the output render pipeline
			context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

			// clear the back buffer with the background color and clear the depth buffer
			float color[4] = { 0.f, 0.f, 0.f, 1.f };
			context->ClearRenderTargetView(renderTargetView, color);
			context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

			// generate the view matrix based on the camera
			XMVECTOR upVector = XMLoadFloat3(&up);
			XMVECTOR lookAtVector = XMLoadFloat3(&forward);
			XMVECTOR positionVector = XMLoadFloat3(&position);
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
			// rotate the forward and up according to the camera rotation
			lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
			upVector = XMVector3TransformCoord(upVector, rotationMatrix);
			// move the forward vector to the target position
			lookAtVector = XMVectorAdd(positionVector, lookAtVector);
			// create the view matrix
			XMMATRIX viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);


			// add rotation to the sphere
			worldMatrix = XMMatrixRotationRollPitchYaw(0.f, time() * -0.3f, 0.f);


			// stage the sphere's buffers as the ones to use
			// set the vertex buffer to active in the input assembler
			unsigned stride = sizeof(VertexShaderInput);
			unsigned offset = 0;
			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			// set the index buffer to active in the input assembler
			context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// render the model using the lit texture shader

			// lock the matrixBuffer, set the new matrices inside it, and then unlock it
			// shaders must receive transposed matrices in DirectX11
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			ThrowIfFailed(context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
			auto matrixDataPtr = (MatrixBufferType*)mappedResource.pData;
			matrixDataPtr->world = XMMatrixTranspose(worldMatrix);
			matrixDataPtr->view = XMMatrixTranspose(viewMatrix);
			matrixDataPtr->projection = XMMatrixTranspose(projectionMatrix);
			context->Unmap(matrixBuffer, 0);

			// set the materialBuffer information
			ThrowIfFailed(context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
			auto materialDataPtr = (MaterialBufferType*)mappedResource.pData;
			materialDataPtr->materialAmbient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			materialDataPtr->materialDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			materialDataPtr->materialSpecular = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
			context->Unmap(materialBuffer, 0);

			// set the lightingBuffer information
			ThrowIfFailed(context->Map(lightingBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
			auto lightingDataPtr = (LightingBufferType*)mappedResource.pData;
			lightingDataPtr->lightAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
			lightingDataPtr->lightDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			lightingDataPtr->lightDirection = XMFLOAT4(0.70710678118f, 0.0f, 0.70710678118f, 0.0f);
			lightingDataPtr->lightSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			XMStoreFloat4(&lightingDataPtr->viewPosition, positionVector);
			context->Unmap(lightingBuffer, 0);

			// update the vertex and pixel shader constant buffers
			// they're in the constantBuffers in the order: matrix, material, lighting
			// the vertex shader needs matrix, material, lighting in its 0, 1, 2 spots
			context->VSSetConstantBuffers(0, 3, constantBuffers);
			// the pixel shader just needs material, lighting in its 1, 2 spots
			context->PSSetConstantBuffers(1, 2, &(constantBuffers[1]));

			// give the pixel shader the cube texture
			context->PSSetShaderResources(0, 1, &worldTexture->shaderResourceView);

			// set the vertex input layout
			context->IASetInputLayout(litTexture.inputLayout);

			// set the vertex and pixel shaders that will be used to render this triangle
			context->VSSetShader(litTexture.vertexShader, NULL, 0);
			context->PSSetShader(litTexture.pixelShader, NULL, 0);
			context->PSSetSamplers(0, 1, &samplerState);

			// render the sphere
			context->DrawIndexed(indexCount, 0, 0);

			// show the fps
			++framesDrawn;
			fpsElapsed += elapsed;
			if (fpsElapsed >= fpsUpdateDelay) {
				swprintf_s(fps, L"fps: %.2f   ", framesDrawn / fpsElapsed);
				fpsLength = (UINT32)wcslen(fps);
				fpsUpdateDelay = min(0.5f, sqrt(fpsElapsed / framesDrawn));
				fpsElapsed = 0;
				framesDrawn = 0;
			}
			d2dContext->BeginDraw();
			d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
			d2dContext->DrawTextW(fps, fpsLength, textFormat, D2D1::RectF(10.f, 10.f, 410.f, 110.f), whiteBrush);
			d2dContext->EndDraw();


			// Present the back buffer to the screen since rendering is complete.
			swapChain->Present(vsync ? 1 : 0, 0);
		}
	}

	// clean up
	worldTexture->Release();
	Uber::I().resourceManager->Terminate();
	delete Uber::I().resourceManager;
	if (whiteBrush) whiteBrush->Release();
	if (d2dContext) d2dContext->Release();
	if (d2dDevice) d2dDevice->Release();
	if (d2dFactory) d2dFactory->Release();
	if (swapChain && !windowed) swapChain->SetFullscreenState(false, NULL);
	if (vertexBuffer) vertexBuffer->Release();
	if (indexBuffer) indexBuffer->Release();
	if (samplerState) samplerState->Release();
	if (lightingBuffer) lightingBuffer->Release();
	if (materialBuffer) materialBuffer->Release();
	if (matrixBuffer) matrixBuffer->Release();
	//if (textureShaderInputLayout) textureShaderInputLayout->Release();
	//if (textureVertexShader) textureVertexShader->Release();
	//if (texturePixelShader) texturePixelShader->Release();
	//if (colorShaderInputLayout) colorShaderInputLayout->Release();
	//if (colorVertexShader) colorVertexShader->Release();
	if (rasterState) rasterState->Release();
	//if (colorPixelShader) colorPixelShader->Release();
	if (depthStencilView) depthStencilView->Release();
	if (depthStencilState) depthStencilState->Release();
	if (depthStencilBuffer) depthStencilBuffer->Release();
	if (renderTargetView) renderTargetView->Release();
	if (context) context->Release();
	if (device) device.Get()->Release();
	if (swapChain) swapChain->Release();

	// return this part of the WM_QUIT message to Windows
	return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// sort through and find what code to run for the message given
	switch (message) {
		// this message is read when the window is closed
		case WM_DESTROY:
		{
			// close the application entirely
			PostQuitMessage(0);
			return 0;
		} break;
	}

	// handle any messages that the switch statement didn't
	return DefWindowProc(hWnd, message, wParam, lParam);
}