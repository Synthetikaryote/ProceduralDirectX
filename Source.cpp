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
#include "ConstantBuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "Model.h"
#include "RenderTarget.h"
#include "Transform.h"

// text rendering
#include <d2d1_2.h>
#include <dwrite.h>

// input
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace DirectX;
using namespace std;
using namespace Microsoft::WRL;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// clear out the window class for use
	ZeroMemory(&Uber::I().wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	Uber::I().wc.cbSize = sizeof(WNDCLASSEX);
	Uber::I().wc.style = CS_HREDRAW | CS_VREDRAW;
	Uber::I().wc.lpfnWndProc = WindowProc;
	Uber::I().wc.hInstance = hInstance;
	Uber::I().wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	Uber::I().wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	Uber::I().wc.lpszClassName = L"WindowClass1";

	// register the window class
	RegisterClassEx(&Uber::I().wc);

	bool vsync = false;
	bool windowed = true;

	Uber::I().screenWidth = static_cast<unsigned>(GetSystemMetrics(SM_CXSCREEN));
	Uber::I().screenHeight = static_cast<unsigned>(GetSystemMetrics(SM_CYSCREEN));
	Uber::I().windowWidth = windowed ? Uber::I().screenWidth * 2 / 4 : Uber::I().screenWidth;
	Uber::I().windowHeight = windowed ? Uber::I().screenHeight * 2 / 4 : Uber::I().screenHeight;
	Uber::I().windowLeft = windowed ? (Uber::I().screenWidth - Uber::I().windowWidth) / 2 : 0;
	Uber::I().windowTop = windowed ? (Uber::I().screenHeight - Uber::I().windowHeight) / 2 : 0;
	RECT wr = { 0, 0, Uber::I().windowWidth, Uber::I().windowHeight };    // set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

	// create the window and use the result as the handle
	Uber::I().hWnd = CreateWindowEx(NULL,
		L"WindowClass1",    // name of the window class
		L"ProceduralDirectX",   // title of the window
		WS_OVERLAPPEDWINDOW,    // window style
		Uber::I().windowLeft,    // x-position of the window
		Uber::I().windowTop,    // y-position of the window
		wr.right - wr.left,    // width of the window
		wr.bottom - wr.top,    // height of the window
		NULL,    // we have no parent window, NULL
		NULL,    // we aren't using menus, NULL
		hInstance,    // application handle
		NULL);    // used with multiple windows, NULL

	// display the window on the screen
	ShowWindow(Uber::I().hWnd, nCmdShow);

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
	swapChainDesc.BufferDesc.Width = Uber::I().windowWidth;
	swapChainDesc.BufferDesc.Height = Uber::I().windowHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = Uber::I().hWnd;
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
			if (displayModeList[i].Width == Uber::I().windowWidth && displayModeList[i].Height == Uber::I().windowHeight) {
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
	device->CreateRenderTargetView(backBuffer, nullptr, &Uber::I().renderTargetView);

	adapterOutput->Release();
	adapter->Release();
	factory->Release();
	backBuffer->Release();

	// create the depth stencil buffer
	ID3D11Texture2D* depthStencilBuffer = nullptr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
	depthBufferDesc.Width = Uber::I().windowWidth;
	depthBufferDesc.Height = Uber::I().windowHeight;
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
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &Uber::I().depthStencilView);

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
	Uber::I().viewport.Width = static_cast<float>(Uber::I().windowWidth);
	Uber::I().viewport.Height = static_cast<float>(Uber::I().windowHeight);
	Uber::I().viewport.MinDepth = 0.0f;
	Uber::I().viewport.MaxDepth = 1.0f;
	Uber::I().viewport.TopLeftX = 0.0f;
	Uber::I().viewport.TopLeftY = 0.0f;
	context->RSSetViewports(1, &Uber::I().viewport);

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

	// create the world matrix
	XMMATRIX worldMatrix = XMMatrixIdentity();

	// create the resource manager
	Uber::I().resourceManager = new ResourceManager();

	// constant buffers that are in the shaders
	// these lines create them in their constructor according to the type information
	ConstantBuffer<MatrixBufferType> matrixBuffer;
	ConstantBuffer<MaterialBufferType> materialBuffer;
	ConstantBuffer<LightingBufferType> lightingBuffer;
	ConstantBuffer<BrushBufferType> brushBuffer;
	ConstantBuffer<PostProcessBufferType> postProcessBuffer;

	// shaders
	// post processing
	// create the render target for the scene to render first, to allow post processing
	RenderTarget sceneTarget(Uber::I().windowWidth, Uber::I().windowHeight, DXGI_FORMAT_B8G8R8A8_UNORM);
	Mesh* screenMesh = new Mesh();
	screenMesh->vertexCount = 4;
	screenMesh->vertices = new Vertex[4];
	screenMesh->vertices[0].position = XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	screenMesh->vertices[1].position = XMFLOAT4(-1.0f, 1.0f, 0.0f, 1.0f);
	screenMesh->vertices[2].position = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	screenMesh->vertices[3].position = XMFLOAT4(1.0f, -1.0f, 0.0f, 1.0f);
	screenMesh->vertices[0].texture = XMFLOAT3(0.0f, 1.0f, 0.0f);
	screenMesh->vertices[1].texture = XMFLOAT3(0.0f, 0.0f, 0.0f);
	screenMesh->vertices[2].texture = XMFLOAT3(1.0f, 0.0f, 0.0f);
	screenMesh->vertices[3].texture = XMFLOAT3(1.0f, 1.0f, 0.0f);
	for (int i = 0; i < 4; ++i) {
		screenMesh->vertices[i].normal = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		screenMesh->vertices[i].tangent = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	screenMesh->indexCount = 6;
	screenMesh->indices = new unsigned long[6]{0, 1, 2, 0, 2, 3};
	screenMesh->CreateBuffers();
	Model* screen = new Model();
	screen->meshes.push_back(screenMesh);
	++screenMesh->refCount;
	D3D11_SAMPLER_DESC postProcessSamplerDesc = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0.0f, 1, D3D11_COMPARISON_ALWAYS, { 0, 0, 0, 0 }, 0, D3D11_FLOAT32_MAX };
	Shader* postProcessShader = Shader::LoadShader("PostProcessVS.cso", "PostProcessPS.cso", { { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 } }, postProcessSamplerDesc);
	postProcessBuffer.data.windowSize = XMFLOAT2(Uber::I().windowWidth, Uber::I().windowHeight);
	postProcessBuffer.UpdateSubresource();
	postProcessShader->pixelShaderConstantBuffers = { lightingBuffer.buffer, postProcessBuffer.buffer };

	// lit texture
	D3D11_SAMPLER_DESC samplerDesc = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0.0f, 1, D3D11_COMPARISON_ALWAYS, { 0, 0, 0, 0 }, 0, D3D11_FLOAT32_MAX };
	Shader* litTexture = Shader::LoadShader("LitTextureVS.cso", "LitTexturePS.cso", { { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 } }, samplerDesc);
	// set up the vertex and pixel shader constant buffers
	// the vertex shader needs matrix, material, lighting in its 0, 1, 2 spots
	litTexture->vertexShaderConstantBuffers = { matrixBuffer.buffer, materialBuffer.buffer, lightingBuffer.buffer };
	// the pixel shader just needs material and lighting in its 0, 1 spots
	litTexture->pixelShaderConstantBuffers = { materialBuffer.buffer, lightingBuffer.buffer, brushBuffer.buffer };

	// make a cube sphere
	// texture
	// note: go to planetpixelemporium for textures
	//vector<string> paths(6);
	//for (int i = 0; i < 6; ++i) {
	//	char fileName[40];
	//	sprintf_s(fileName, "earth_%c.tga", "rludfb"[i]);
	//	paths[i] = string(fileName);
	//}
	//Mesh* world = Mesh::LoadCubeSphere(20);
	Model* world = new Model();
	Mesh* worldMesh = Mesh::LoadSphere(500, 250);
	//Texture* diffuseTexture = Texture::LoadCube(paths);
	Texture* heightTexture = Texture::Load(string("8081-earthbump4k.jpg"));
	Texture* diffuseTexture = Texture::Load(string("8081-earthmap4k.jpg"));
	Texture* specularTexture = Texture::Load(string("8081-earthspec4k.jpg"));
	Texture* normalTexture = Texture::Load(string("8081-earthnormal4k.jpg"));
	TextureBinding heightBinding = { heightTexture, ShaderTypeVertex, 0 };
	TextureBinding diffuseBinding = { diffuseTexture, ShaderTypePixel, diffuseTexture->isTextureCube ? 1 : 0 };
	TextureBinding specularBinding = { specularTexture, ShaderTypePixel, 2 };
	TextureBinding normalBinding = { normalTexture, ShaderTypePixel, 3 };
	worldMesh->textureBindings = { heightBinding, diffuseBinding, specularBinding, normalBinding };
	worldMesh->shader = litTexture;
	world->meshes.push_back(worldMesh);

	//auto* duckTexture = Texture::Load(string("Models/duck.tga"));
	//Mesh* duck = Mesh::LoadFromFile("duck.txt");
	vector<Model*> models = { world };

	// set up the camera
	Uber::I().camera = new Camera();
	Uber::I().camera->binds = { DIK_E, DIK_S, DIK_D, DIK_F, DIK_SPACE, DIK_LCONTROL };
	Uber::I().camera->sensitivity = { 0.002f, 0.002f, 0.002f };
	Uber::I().camera->SetFocus(world);

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

	// input
	// keyboard
	IDirectInput8* directInput;
	ThrowIfFailed(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, NULL));
	IDirectInputDevice8* keyboard;
	ThrowIfFailed(directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL));
	ThrowIfFailed(keyboard->SetDataFormat(&c_dfDIKeyboard));
	ThrowIfFailed(keyboard->SetCooperativeLevel(Uber::I().hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));
	keyboard->Acquire();
	// mouse
	IDirectInputDevice8* mouse;
	ThrowIfFailed(directInput->CreateDevice(GUID_SysMouse, &mouse, NULL));
	ThrowIfFailed(mouse->SetDataFormat(&c_dfDIMouse));
	ThrowIfFailed(mouse->SetCooperativeLevel(Uber::I().hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
	mouse->Acquire();

	Uber::I().context->OMSetRenderTargets(1, &Uber::I().renderTargetView, Uber::I().depthStencilView);


	// main loop
	MSG msg = { 0 };
	unsigned framesDrawn = 0;
	float lastFrameTime = 0;
	float fpsElapsed = 1.f;
	float fpsUpdateDelay = 0.5f;
	UINT32 fpsLength = 0;
	wchar_t fps[255];
	wchar_t message[255];
	swprintf_s(message, L"");
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

			// handle all window messages before rendering anything
			continue;
		}

		float elapsed = time() - lastFrameTime;
		lastFrameTime = time();

		// escape to quit
		if (IsKeyDown(DIK_ESCAPE))
			break;

		if (IsKeyDown(DIK_1))
			Uber::I().camera->SetFocus(world);
		if (IsKeyDown(DIK_2))
			Uber::I().camera->SetFocus(nullptr);

		// read the input
		HRESULT result = keyboard->GetDeviceState(sizeof(Uber::I().keyboardState), (LPVOID)&Uber::I().keyboardState);
		if (FAILED(result) && (result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
			keyboard->Acquire();
		result = mouse->GetDeviceState(sizeof(Uber::I().mouseState), (LPVOID)&Uber::I().mouseState);
		if (FAILED(result) && (result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
			mouse->Acquire();
		POINT windowMousePos;
		if (Uber::I().cursorVisible) {
			GetCursorPos(&windowMousePos);
			ScreenToClient(Uber::I().hWnd, &windowMousePos);
			Uber::I().mouseX = windowMousePos.x;
			Uber::I().mouseY = windowMousePos.y;
		}

		// update the camera
		Uber::I().camera->Update(elapsed);
		XMMATRIX rotationMatrix = XMLoadFloat4x4(&Uber::I().camera->rot);
		XMMATRIX viewMatrix = XMLoadFloat4x4(&Uber::I().camera->view);
		XMMATRIX projectionMatrix = XMLoadFloat4x4(&Uber::I().camera->proj);

		RaycastResult rayResult = Uber::I().camera->ScreenRaycastToModelSphere(world);
		if (rayResult.didHit) {
			float closeness = powf(Uber::I().camera->zoomBase, Uber::I().camera->focusLinearZoom);
			brushBuffer.data.cursorFlags = 1;
			brushBuffer.data.cursorRadiusSq = ((0.01f + closeness) * 0.1f + 0.01f) * ((0.01f + closeness) * 0.1f + 0.01f);
			//swprintf_s(message, L"cursorRadius: %.8f", brushBuffer.data.cursorRadiusSq);
			brushBuffer.data.cursorLineThickness = (1.0f - Uber::I().camera->focusLinearZoom) * 0.1f;
			brushBuffer.data.cursorPosition = rayResult.hitLocation;
			brushBuffer.UpdateSubresource();
		}
		else if (brushBuffer.data.cursorFlags & 1) {
			brushBuffer.data.cursorFlags = 0;
			brushBuffer.UpdateSubresource();
			//swprintf_s(message, L"");
		}

		// update the constant buffers that are constant for all meshes
		// shaders must receive transposed matrices in DirectX11
		matrixBuffer.data.world = XMMatrixTranspose(worldMatrix);
		matrixBuffer.data.view = XMMatrixTranspose(viewMatrix);
		matrixBuffer.data.projection = XMMatrixTranspose(projectionMatrix);
		matrixBuffer.UpdateSubresource();

		// set the lightingBuffer information
		lightingBuffer.data.lightAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
		lightingBuffer.data.lightDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		lightingBuffer.data.lightDirection = XMFLOAT4(0.70710678118f, 0.0f, 0.70710678118f, 0.0f);
		lightingBuffer.data.lightSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		XMVECTOR positionVector = XMLoadFloat3(&Uber::I().camera->position);
		XMStoreFloat4(&lightingBuffer.data.viewPosition, positionVector);
		lightingBuffer.UpdateSubresource();

		// clear the back buffer with the background color and clear the depth buffer
		float color[4] = { 0.f, 0.f, 0.f, 1.f };
		context->ClearRenderTargetView(Uber::I().renderTargetView, color);
		context->ClearDepthStencilView(Uber::I().depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// render everything to the scene render target to allow post processing
		sceneTarget.BeginRender();

		for (auto* model : models) {
			for (auto* mesh : model->meshes) {
				// add rotation to the sphere
				if (Uber::I().camera->focus) {
					XMVECTOR upVector = XMLoadFloat3(&Uber::I().camera->up);
					XMVECTOR rightVector = XMLoadFloat3(&XMFLOAT3(1.0f, 0.0f, 0.0f));
					worldMatrix = XMMatrixRotationAxis(upVector, Uber::I().camera->focusYaw) * XMMatrixRotationAxis(rightVector, -Uber::I().camera->focusPitch);
				}
				else {
					worldMatrix = XMMatrixIdentity();
				}

				// render the model using the lit texture shader

				mesh->shader->SwitchTo();

				// set the material settings
				materialBuffer.data.materialAmbient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				materialBuffer.data.materialDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				materialBuffer.data.materialSpecular = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
				materialBuffer.data.vsSlotsUsed = 0;
				materialBuffer.data.psSlotsUsed = 0;
				for (auto& binding : mesh->textureBindings) {
					switch (binding.shaderType) {
						case ShaderTypeVertex:
							materialBuffer.data.vsSlotsUsed |= 1 << binding.shaderSlot;
							break;
						case ShaderTypePixel:
							materialBuffer.data.psSlotsUsed |= 1 << binding.shaderSlot;
							break;
					}
				}
				materialBuffer.data.psFlags = PSFlags::CelShading;
				materialBuffer.UpdateSubresource();

				mesh->Draw();
			}
		}

		// switch back to the back buffer
		sceneTarget.EndRender();

		// post processing
		postProcessShader->SwitchTo();
		sceneTarget.BindTexture(0);
		screen->meshes[0]->Draw();
		sceneTarget.UnbindTexture(0);

		// show the fps
		++framesDrawn;
		fpsElapsed += elapsed;
		if (fpsElapsed >= fpsUpdateDelay) {
			swprintf_s(fps, L"fps: %.2f", framesDrawn / fpsElapsed);
			fpsLength = (UINT32)wcslen(fps);
			fpsUpdateDelay = min(0.5f, sqrt(fpsElapsed / framesDrawn));
			fpsElapsed = 0;
			framesDrawn = 0;
		}
		d2dContext->BeginDraw();
		d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
		d2dContext->DrawTextW(fps, fpsLength, textFormat, D2D1::RectF(10.f, 10.f, 410.f, 110.f), whiteBrush);
		d2dContext->DrawTextW(message, (UINT32)wcslen(message), textFormat, D2D1::RectF(10.f, 120.f, 810.f, 110.f), whiteBrush);
		d2dContext->EndDraw();

		// Present the back buffer to the screen since rendering is complete.
		swapChain->Present(vsync ? 1 : 0, 0);
	}

	// clean up
	mouse->Unacquire();
	mouse->Release();
	keyboard->Unacquire();
	keyboard->Release();
	directInput->Release();
	for (auto model : models) delete model;
	delete screen;
	postProcessShader->Release();
	Uber::I().resourceManager->Terminate();
	delete Uber::I().resourceManager;
	delete Uber::I().camera;
	if (whiteBrush) whiteBrush->Release();
	if (d2dContext) d2dContext->Release();
	if (d2dDevice) d2dDevice->Release();
	if (d2dFactory) d2dFactory->Release();
	if (swapChain && !windowed) swapChain->SetFullscreenState(false, NULL);
	//if (textureShaderInputLayout) textureShaderInputLayout->Release();
	//if (textureVertexShader) textureVertexShader->Release();
	//if (texturePixelShader) texturePixelShader->Release();
	//if (colorShaderInputLayout) colorShaderInputLayout->Release();
	//if (colorVertexShader) colorVertexShader->Release();
	if (rasterState) rasterState->Release();
	//if (colorPixelShader) colorPixelShader->Release();
	if (Uber::I().depthStencilView) Uber::I().depthStencilView->Release();
	if (depthStencilState) depthStencilState->Release();
	if (depthStencilBuffer) depthStencilBuffer->Release();
	if (Uber::I().renderTargetView) Uber::I().renderTargetView->Release();
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
		case WM_SETFOCUS:
			Uber::I().windowIsFocused = true;
			break;
		case WM_KILLFOCUS:
			if (!Uber::I().cursorVisible) {
				SetCursorPos(Uber::I().savedMousePos.x, Uber::I().savedMousePos.y);
				ShowCursor(true);
				Uber::I().cursorVisible = true;
			}
			Uber::I().windowIsFocused = false;
			break;
	}

	// handle any messages that the switch statement didn't
	return DefWindowProc(hWnd, message, wParam, lParam);
}