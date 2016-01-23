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

POINT savedMousePos;
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
	wc.hCursor = NULL; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";
	ShowCursor(false);

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
		L"ProceduralDirectX",   // title of the window
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
	float fieldOfView = PI / 4.0f;
	float screenAspect = (float)screenWidth / (float)screenHeight;
	float screenDepth = 1000.0f;
	float screenNear = 0.0001f;
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	// create the world matrix
	XMMATRIX worldMatrix = XMMatrixIdentity();

	// create an orthographic projection matrix for 2D UI rendering.
	XMMATRIX orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	// create the resource manager
	Uber::I().resourceManager = new ResourceManager();

	// constant buffers that are in the shaders
	// these lines create them in their constructor according to the type information
	ConstantBuffer<MatrixBufferType> matrixBuffer;
	ConstantBuffer<MaterialBufferType> materialBuffer;
	ConstantBuffer<LightingBufferType> lightingBuffer;

	// shaders
	D3D11_SAMPLER_DESC samplerDesc = {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, 0.0f, 1, D3D11_COMPARISON_ALWAYS, {0, 0, 0, 0}, 0, D3D11_FLOAT32_MAX};
	Shader* litTexture = Shader::LoadShader("LitTextureVS.cso", "LitTexturePS.cso", {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	}, samplerDesc
	);
	// set up the vertex and pixel shader constant buffers
	// the vertex shader needs matrix, material, lighting in its 0, 1, 2 spots
	litTexture->vertexShaderConstantBuffers = {matrixBuffer.buffer, materialBuffer.buffer, lightingBuffer.buffer};
	// the pixel shader just needs material and lighting in its 0, 1 spots
	litTexture->pixelShaderConstantBuffers = {materialBuffer.buffer, lightingBuffer.buffer};

	// make a cube sphere
	// texture
	// note: go to planetpixelemporium for textures
	vector<string> paths(6);
	for (int i = 0; i < 6; ++i) {
		char fileName[40];
		sprintf_s(fileName, "earth_%c.tga", "rludfb"[i]);
		paths[i] = string(fileName);
	}
	Mesh* world = Mesh::LoadCubeSphere(20);
	Texture* diffuseTexture = Texture::LoadCube(paths);
	TextureBinding diffuseBinding = {diffuseTexture, diffuseTexture->isTextureCube ? 1 : 0};
	world->textureBindings = {diffuseBinding};
	world->shader = litTexture;
	
	//auto* duckTexture = Texture::Load(string("Models/duck.tga"));
	//Mesh* duck = Mesh::LoadFromFile("duck.txt");
	vector<Mesh*> meshes = {world};

	// set up the camera
	XMFLOAT3 position(0.f, 0.f, -3.f);
	XMFLOAT3 rotation(0.f, 0.f, 0.f);
	XMFLOAT3 up(0.f, 1.f, 0.f);
	XMFLOAT3 forward(0.f, 0.f, 1.f);
	XMFLOAT3 velocity(0.f, 0.f, 0.f);
	float yaw = 0.f, pitch = 0.f;


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
	ThrowIfFailed(keyboard->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));
	keyboard->Acquire();

	// mouse
	IDirectInputDevice8* mouse;
	ThrowIfFailed(directInput->CreateDevice(GUID_SysMouse, &mouse, NULL));
	ThrowIfFailed(mouse->SetDataFormat(&c_dfDIMouse));
	ThrowIfFailed(mouse->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));
	mouse->Acquire();


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

			// read the input
			HRESULT result = keyboard->GetDeviceState(sizeof(Uber::I().keyboardState), (LPVOID)&Uber::I().keyboardState);
			if (FAILED(result) && (result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
				keyboard->Acquire();
			result = mouse->GetDeviceState(sizeof(Uber::I().mouseState), (LPVOID)&Uber::I().mouseState);
			if (FAILED(result) && (result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
				mouse->Acquire();
			Uber::I().mouseX = max(0, min(Uber::I().mouseX + Uber::I().mouseState.lX, screenWidth));
			Uber::I().mouseY = max(0, min(Uber::I().mouseY + Uber::I().mouseState.lY, screenHeight));
			if (GetActiveWindow() == hWnd) {
				RECT r;
				GetWindowRect(hWnd, &r);
				SetCursorPos((float)(r.right - r.left) * 0.5f, (float)(r.bottom - r.top) * 0.5f);
			}

			// escape to quit
			if (Uber::IsKeyDown(DIK_ESCAPE))
				break;

			const float sensitivity = 0.0002f;
			yaw = fmodf(yaw + sensitivity * Uber::I().mouseState.lX, TWOPI);
			pitch = max(-HALFPI, min(fmodf(pitch + sensitivity * Uber::I().mouseState.lY, TWOPI), HALFPI));

			float speed = 2.f;
			float x = 0.f, y = 0.f, z = 0.f;
			x = (Uber::IsKeyDown(DIK_S) ? -1.f : 0.f) + (Uber::IsKeyDown(DIK_F) ? 1.f : 0.f);
			y = (Uber::IsKeyDown(DIK_LCONTROL) ? -1.f : 0.f) + (Uber::IsKeyDown(DIK_SPACE) ? 1.f : 0.f);
			z = (Uber::IsKeyDown(DIK_D) ? -1.f : 0.f) + (Uber::IsKeyDown(DIK_E) ? 1.f : 0.f);
			float lenSq = x * x + y * y + z * z;
			if (lenSq > 0.f) {
				float lenInv = 1.f / sqrt(lenSq);
				x *= lenInv; y *= lenInv; z *= lenInv;
				float py = y * cos(pitch) - z * sin(pitch), pz = y * sin(pitch) + z * cos(pitch);
				float yx = x * cos(yaw) + pz * sin(yaw), yz = -x * sin(yaw) + pz * cos(yaw);
				x = yx; y = py; z = yz;
				float mult = speed * elapsed;
				x *= mult; y *= mult; z *= mult;
				position.x += x; position.y += y; position.z += z;
			}


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
			XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.f);
			// rotate the forward and up according to the camera rotation
			lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
			upVector = XMVector3TransformCoord(upVector, rotationMatrix);
			// move the forward vector to the target position
			lookAtVector = XMVectorAdd(positionVector, lookAtVector);
			// create the view matrix
			XMMATRIX viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

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
			XMStoreFloat4(&lightingBuffer.data.viewPosition, positionVector);
			lightingBuffer.UpdateSubresource();

			for (auto* mesh : meshes) {
				// add rotation to the sphere
				//worldMatrix = XMMatrixRotationRollPitchYaw(0.f, time() * -0.3f, 0.f);

				// stage the mesh's buffers as the ones to use
				// set the vertex buffer to active in the input assembler
				
				unsigned stride = sizeof(Vertex);
				unsigned offset = 0;
				context->IASetVertexBuffers(0, 1, &(mesh->vertexBuffer), &stride, &offset);
				// set the index buffer to active in the input assembler
				context->IASetIndexBuffer(mesh->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
				// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				// render the model using the lit texture shader

				mesh->shader->SwitchTo();

				// set the material settings
				materialBuffer.data.materialAmbient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				materialBuffer.data.materialDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				materialBuffer.data.materialSpecular = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
				materialBuffer.data.slotsUsed = 0;
				for (auto& binding : mesh->textureBindings)
					materialBuffer.data.slotsUsed |= 1 << binding.shaderSlot;
				materialBuffer.UpdateSubresource();

				mesh->Draw();
			}

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
	mouse->Unacquire();
	mouse->Release();
	keyboard->Unacquire();
	keyboard->Release();
	directInput->Release();
	for (auto* mesh : meshes)
		mesh->Release();
	Uber::I().resourceManager->Terminate();
	delete Uber::I().resourceManager;
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
		case WM_SETFOCUS:
			GetCursorPos(&savedMousePos);
			break;
		case WM_KILLFOCUS:
			SetCursorPos(savedMousePos.x, savedMousePos.y);
			break;
	}

	// handle any messages that the switch statement didn't
	return DefWindowProc(hWnd, message, wParam, lParam);
}