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
#include "DepthStencilState.h"
#include "HeightTree.h"

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
	Uber::I().windowWidth = windowed ? Uber::I().screenWidth * 3 / 4 : Uber::I().screenWidth;
	Uber::I().windowHeight = windowed ? Uber::I().screenHeight * 3 / 4 : Uber::I().screenHeight;
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
	Uber::I().depthStencilState = new DepthStencilState(true, true);

	// create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &Uber::I().depthStencilView);

	// custom raster options, like draw as wireframe
	ID3D11RasterizerState* rasterStateSolid = nullptr;
	ID3D11RasterizerState* rasterStateWireframe = nullptr;
	D3D11_RASTERIZER_DESC rasterDescSolid, rasterDescWireframe;
	rasterDescSolid.AntialiasedLineEnable = false;
	rasterDescSolid.CullMode = D3D11_CULL_BACK;
	rasterDescSolid.DepthBias = 0;
	rasterDescSolid.DepthBiasClamp = 0.0f;
	rasterDescSolid.DepthClipEnable = true;
	rasterDescSolid.FillMode = D3D11_FILL_SOLID;
	rasterDescSolid.FrontCounterClockwise = false;
	rasterDescSolid.MultisampleEnable = false;
	rasterDescSolid.ScissorEnable = false;
	rasterDescSolid.SlopeScaledDepthBias = 0.0f;
	rasterDescWireframe = rasterDescSolid;
	rasterDescWireframe.FillMode = D3D11_FILL_WIREFRAME;
	device->CreateRasterizerState(&rasterDescSolid, &rasterStateSolid);
	device->CreateRasterizerState(&rasterDescWireframe, &rasterStateWireframe);
	context->RSSetState(rasterStateSolid);

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

	// heights
	unsigned meshSize = 32;
	Uber::I().zoomStep = 8;
	Uber::I().heights = new HeightTree();
	Uber::I().heights->height = 1.0f;
	Uber::I().heights->s = heightSMax;

	// create the resource manager
	Uber::I().resourceManager = new ResourceManager();

	// constant buffers that are in the shaders
	// these lines create them in their constructor according to the type information
	ConstantBuffer<MatrixBufferType> matrixBuffer;
	ConstantBuffer<MaterialBufferType> materialBuffer;
	ConstantBuffer<LightingBufferType> lightingBuffer;
	ConstantBuffer<BrushBufferType> brushBuffer;
	ConstantBuffer<PostProcessBufferType> postProcessBuffer;
	ConstantBuffer<DepthMapBufferType> depthMapBuffer;

	// this information doesn't change
	lightingBuffer.data.lightAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	lightingBuffer.data.lightDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lightingBuffer.data.lightDirection = XMFLOAT4(0.70710678118f, 0.0f, 0.70710678118f, 0.0f);
	lightingBuffer.data.lightSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);


	// shaders
	// post processing
	// create the render target for the scene to render first, to allow post processing
	RenderTarget sceneTarget(Uber::I().windowWidth, Uber::I().windowHeight, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
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
	Mesh* worldMesh = Mesh::LoadPartialSphere(meshSize, meshSize, 0.0f, TWOPI, 0.0f, PI, D3D11_CPU_ACCESS_WRITE);
	//Texture* diffuseTexture = Texture::LoadCube(paths);
	Texture* heightTexture = Texture::Load(string("8081-earthbump4k.jpg"));
	Texture* diffuseTexture = Texture::Load(string("8081-earthmap4k.jpg"));
	Texture* specularTexture = Texture::Load(string("8081-earthspec4k.jpg"));
	Texture* normalTexture = Texture::Load(string("8081-earthnormal4k.jpg"));
	TextureBinding heightBinding = { heightTexture, ShaderTypeVertex, 0 };
	TextureBinding diffuseBinding = { diffuseTexture, ShaderTypePixel, diffuseTexture->isTextureCube ? 1 : 0 };
	TextureBinding specularBinding = { specularTexture, ShaderTypePixel, 2 };
	TextureBinding normalBinding = { normalTexture, ShaderTypePixel, 3 };
	TextureBinding heightBindingPS = {heightTexture, ShaderTypePixel, 5};
	//heightTexture->AddRef();
	worldMesh->textureBindings = { heightBinding, diffuseBinding, specularBinding, normalBinding/*, heightBindingPS*/ };
	worldMesh->depthMapTextureBindings = {heightBinding};
	worldMesh->shader = litTexture;
	world->meshes.push_back(worldMesh);

	auto* duckTexture = Texture::Load(string("Models/duck.tga"));
	Mesh* duckMesh = Mesh::LoadFromFile("duck.txt");
	duckMesh->shader = litTexture;
	litTexture->AddRef();
	TextureBinding duckDiffuseBinding = {duckTexture, ShaderTypePixel, duckTexture->isTextureCube ? 1 : 0};
	duckMesh->textureBindings = {duckDiffuseBinding};
	Model* duck = new Model();
	duck->meshes.push_back(duckMesh);
	duck->transform->x = -1.4f;
	duck->transform->y = -0.25f;
	duck->transform->z = -1.4f;
	duck->transform->scaleX = 0.4f;
	duck->transform->scaleY = 0.4f;
	duck->transform->scaleZ = 0.4f;
	vector<Model*> models = { world, duck };

	// set up the camera
	Uber::I().camera = new Camera(PI / 4.0f, static_cast<float>(Uber::I().windowWidth) / Uber::I().windowHeight, 0.00001f, 1000.0f);
	Uber::I().camera->binds = { DIK_E, DIK_S, DIK_D, DIK_F, DIK_SPACE, DIK_LCONTROL };
	Uber::I().camera->sensitivity = { 0.002f, 0.002f, 0.002f };
	Uber::I().camera->SetFocus(world);

	// camera for shadows
	Uber::I().lightCamera = new Camera(PI / 32.0f, 1.0f, 0.1f, 100.0f);
	Uber::I().lightCamera->position = XMFLOAT3(-15.0f, 0.0f, -15.0f);
	Uber::I().lightCamera->yaw = PI * 0.25f;
	Uber::I().lightCamera->Update(0.0f);
	RenderTarget depthMap(2048, 2048, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	// shader for shadows
	Shader* depthMapShader = Shader::LoadShader("DepthMapVS.cso", "", {{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}}, samplerDesc);
	depthMapShader->vertexShaderConstantBuffers = {depthMapBuffer.buffer};
	// shadow transform matrix
	// converts [-1:1, -1:1] to [0:1, 0:1]
	XMFLOAT4X4 matNDCtoUV(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);
	vector<Model*> shadowCasters = {duck};
	// add the depthMap to the world bindings
	Texture* depthMapTexture = new Texture();
	depthMapTexture->shaderResourceView = depthMap.shaderResourceView;
	TextureBinding depthMapBinding = {depthMapTexture, ShaderTypePixel, 4};
	depthMapTexture->AddRef();
	worldMesh->textureBindings.push_back(depthMapBinding);
	D3D11_SAMPLER_DESC shadowSamplerDesc = {D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, 0.0f, 1, D3D11_COMPARISON_ALWAYS, {1, 1, 1, 1}, 0, D3D11_FLOAT32_MAX};
	ID3D11SamplerState* shadowSamplerState;
	ThrowIfFailed(device->CreateSamplerState(&shadowSamplerDesc, &shadowSamplerState));

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


	float raiseDir = 1.0f;
	float flattenHeight = 0.0f;
	bool flatten = false;
	bool wireframe = false;
	bool showDepthMap = false;

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

		if (IsKeyDown(DIK_1)) {
			raiseDir = 1.0f;
			flattenHeight = 0.0f;
		}
		else if (IsKeyDown(DIK_2)) {
			raiseDir = -1.0f;
			flattenHeight = 0.0f;
		}
		else if (IsKeyDown(DIK_3)) {
			raiseDir = 0.0f;
			flattenHeight = 0.0f;
		}
		else if (IsKeyDown(DIK_F1)) {
			showDepthMap = false;
			postProcessBuffer.data.flags = 0;
			postProcessBuffer.UpdateSubresource();
		}
		else if (IsKeyDown(DIK_F2)) {
			showDepthMap = true;
			postProcessBuffer.data.flags = PSFlags::ShowDepthBuffer;
			postProcessBuffer.UpdateSubresource();
		}
		else if (IsKeyDown(DIK_F3)) {
			wireframe = false;
		}
		else if (IsKeyDown(DIK_F4)) {
			wireframe = true;
		}
		else if (IsKeyDown(DIK_F5)) {
			Uber::I().camera->SetFocus(nullptr);
			worldMesh->UpdatePartialSphere(64, 32, 0.0f, TWOPI, 0.0f, PI);
			swprintf_s(message, L"full sphere");
		}
		else if (IsKeyDown(DIK_F6)) {
			Uber::I().camera->SetFocus(world);
			//Uber::I().camera->focus = world;
		}

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

		// update the objects
		duck->transform->yaw += 0.2f * elapsed;

		// update the camera
		bool cameraChanged = Uber::I().camera->Update(elapsed);
		//XMMATRIX rotationMatrix = XMLoadFloat4x4(&Uber::I().camera->rot);
		XMMATRIX viewMatrix = XMLoadFloat4x4(&Uber::I().camera->view);
		XMMATRIX projectionMatrix = XMLoadFloat4x4(&Uber::I().camera->proj);

		if (cameraChanged && Uber::I().camera->focus) {
			RaycastResult topLeft = Uber::I().camera->ScreenRaycastToModelSphere(world, 0, 0);
			RaycastResult topRight = Uber::I().camera->ScreenRaycastToModelSphere(world, static_cast<int>(Uber::I().windowWidth), 0);
			RaycastResult topMiddle = Uber::I().camera->ScreenRaycastToModelSphere(world, static_cast<int>(Uber::I().windowWidth) / 2, 0);
			RaycastResult bottomLeft = Uber::I().camera->ScreenRaycastToModelSphere(world, 0, static_cast<int>(Uber::I().windowHeight));
			RaycastResult bottomRight = Uber::I().camera->ScreenRaycastToModelSphere(world, static_cast<int>(Uber::I().windowWidth), static_cast<int>(Uber::I().windowHeight));
			RaycastResult bottomMiddle = Uber::I().camera->ScreenRaycastToModelSphere(world, static_cast<int>(Uber::I().windowWidth) / 2, static_cast<int>(Uber::I().windowHeight));
			if (topLeft.didHit && topMiddle.didHit && topRight.didHit && bottomLeft.didHit && bottomMiddle.didHit && bottomRight.didHit) {
				auto& tl = topLeft.hitLocation, tm = topMiddle.hitLocation, tr = topRight.hitLocation, bl = bottomLeft.hitLocation, bm = bottomMiddle.hitLocation, br = bottomRight.hitLocation;
				XMVECTOR upVector = XMLoadFloat3(&worldUp);
				XMVECTOR rightVector = XMLoadFloat3(&XMFLOAT3(1.0f, 0.0f, 0.0f));
				XMMATRIX worldMatrix = XMMatrixRotationAxis(upVector, Uber::I().camera->focusYaw) * XMMatrixRotationAxis(rightVector, -Uber::I().camera->focusPitch);
				XMMATRIX inverseWorld = XMMatrixInverse(nullptr, worldMatrix);
				// bring the raycast locations back to model space
				for (auto* v : {&tl, &tm, &tr, &bl, &bm, &br})
					XMStoreFloat4(v, XMVector4Transform(XMLoadFloat4(v), inverseWorld));
				float yawTop = atan2(tm.z, tm.x);
				float yawBot = atan2(bm.z, bm.x);
				float yawMin = 0.0f;
				float yawMax = TWOPI;
				float pitchMin = 0.0f;
				float pitchMax = PI;
				float epsilon = 0.001f;
				if (abs(yawTop - yawBot) > epsilon) {
					// looking at top or bottom
					float pitchTop = atan2(-tm.y, sqrtf(tm.x * tm.x + tm.z * tm.z)) + HALFPI;
					float pitchBot = atan2(-bm.y, sqrtf(bm.x * bm.x + bm.z * bm.z)) + HALFPI;
					if (pitchTop < HALFPI) {
						// top
						assert(pitchBot < HALFPI);
						pitchMax = max(max(atan2(-br.y, sqrtf(br.x * br.x + br.z * br.z)) + HALFPI,
							atan2(-bm.y, sqrtf(bm.x * bm.x + bm.z * bm.z)) + HALFPI),
							atan2(-bl.y, sqrtf(bl.x * bl.x + bl.z * bl.z)) + HALFPI);
					}
					else {
						// bottom
						assert(pitchBot >= HALFPI);
						pitchMin = min(min(atan2(-tl.y, sqrtf(tl.x * tl.x + tl.z * tl.z)) + HALFPI,
							atan2(-tm.y, sqrtf(tm.x * tm.x + tm.z * tm.z)) + HALFPI),
							atan2(-tr.y, sqrtf(tr.x * tr.x + tr.z * tr.z)) + HALFPI);
					}
				}
				else {
					auto GetYaw = [](XMFLOAT4& p) -> float {
						float yaw = atan2(p.z, p.x);
						return yaw;
					};
					yawMin = min(GetYaw(tl), GetYaw(bl));
					if (yawMin < 0.0f) yawMin += TWOPI;
					yawMax = max(GetYaw(br), GetYaw(tr));
					if (yawMax < 0.0f) yawMax += TWOPI;
					pitchMin = min(min(atan2(-tl.y, sqrtf(tl.x * tl.x + tl.z * tl.z)) + HALFPI,
						atan2(-tm.y, sqrtf(tm.x * tm.x + tm.z * tm.z)) + HALFPI),
						atan2(-tr.y, sqrtf(tr.x * tr.x + tr.z * tr.z)) + HALFPI);
					pitchMax = max(max(atan2(-br.y, sqrtf(br.x * br.x + br.z * br.z)) + HALFPI,
						atan2(-bm.y, sqrtf(bm.x * bm.x + bm.z * bm.z)) + HALFPI),
						atan2(-bl.y, sqrtf(bl.x * bl.x + bl.z * bl.z)) + HALFPI);
				}
				// round the bounds to the nearest grid point
				float screenStepF = abs(yawMax - yawMin) / TWOPI * heightSMax;
				unsigned screenStep = max(1, pow(2, ceil(log(screenStepF) / log(2))));
				float stepF = abs(yawMax - yawMin) / TWOPI / meshSize * heightSMax;
				unsigned step = max(1, pow(2, ceil(log(stepF) / log(2))));
				unsigned gridYawMin = floor(yawMin / TWOPI * heightSMax / screenStep) * screenStep;
				unsigned gridYawMax = gridYawMin + meshSize * step;
				yawMin = static_cast<float>(gridYawMin) / heightSMax * TWOPI;
				yawMax = min(TWOPI, static_cast<float>(gridYawMax) / heightSMax * TWOPI);
				unsigned gridPitchMin = floor(pitchMin / PI  * heightSMax / screenStep) * screenStep;
				unsigned gridPitchMax = gridPitchMin + meshSize * step;
				pitchMin = static_cast<float>(gridPitchMin) / heightSMax * PI;
				pitchMax = min(PI, static_cast<float>(gridPitchMax) / heightSMax * PI);
				Uber::I().zoomStep = step;
				worldMesh->UpdatePartialSphere(meshSize - 1, meshSize - 1, yawMin, yawMax, pitchMin, pitchMax);
				swprintf_s(message, L"yaw: %.4f-%.4f  pitch: %.4f-%.4f  screenStep: %u  step: %u", yawMin, yawMax, pitchMin, pitchMax, screenStep, step);
			}
			else {
				worldMesh->UpdatePartialSphere(meshSize - 1, meshSize - 1, 0.0f, TWOPI, 0.0f, PI);
				swprintf_s(message, L"full sphere");
			}
		}

		RaycastResult rayResult = Uber::I().camera->ScreenRaycastToModelSphere(world, Uber::I().mouseX, Uber::I().mouseY);
		if (rayResult.didHit) {
			float closeness = powf(Uber::I().camera->zoomBase, Uber::I().camera->focusLinearZoom);
			brushBuffer.data.cursorFlags = 1;
			brushBuffer.data.cursorRadiusSq = ((0.01f + closeness) * 0.1f + 0.01f) * ((0.01f + closeness) * 0.1f + 0.01f);
			//swprintf_s(message, L"hitLocation: %.2f,%.2f,%.2f", rayResult.hitLocation.x, rayResult.hitLocation.y, rayResult.hitLocation.z);
			brushBuffer.data.cursorLineThickness = (1.0f - Uber::I().camera->focusLinearZoom) * 0.1f;
			brushBuffer.data.cursorPosition = rayResult.hitLocation;
			brushBuffer.UpdateSubresource();

			// hold left click to use tool
			if (Uber::I().mouseState.rgbButtons[0]) {
				XMVECTOR upVector = XMLoadFloat3(&worldUp);
				XMVECTOR rightVector = XMLoadFloat3(&XMFLOAT3(1.0f, 0.0f, 0.0f));
				XMMATRIX worldMatrix = XMMatrixRotationAxis(upVector, Uber::I().camera->focusYaw) * XMMatrixRotationAxis(rightVector, -Uber::I().camera->focusPitch);
				XMMATRIX inverseWorld = XMMatrixInverse(nullptr, worldMatrix);
				// bring the raycast location back to model space
				auto& c = rayResult.hitLocation;
				auto cVec = XMVector4Transform(XMLoadFloat4(&c), inverseWorld);
				XMStoreFloat4(&c, cVec);
				vector<Vertex*> affectedVertices;
				float mostExtremeMin = 1000000.0f;
				float mostExtremeMax = -1000000.0f;
				float totalHeight = 0.0f;
				for (unsigned long i = 0; i < world->meshes[0]->vertexCount; ++i) {
					auto& p = world->meshes[0]->vertices[i].position;
					auto& n = world->meshes[0]->vertices[i].normal;
					// compare to the normal, which is the same as the unmodified positon
					float dx = n.x - c.x, dy = n.y - c.y, dz = n.z - c.z;
					float d = dx * dx + dy * dy + dz * dz;
					if (d <= brushBuffer.data.cursorRadiusSq) {
						affectedVertices.push_back(&world->meshes[0]->vertices[i]);
						float vDistSq = p.x * p.x + p.y * p.y + p.z * p.z;
						mostExtremeMin = min(mostExtremeMin, vDistSq);
						mostExtremeMax = max(mostExtremeMax, vDistSq);
						if (raiseDir == 0.0f && flattenHeight == 0.0f) {
							totalHeight += sqrtf(vDistSq);
						}
					}
				}
				if (affectedVertices.size() > 0) {
					float dist = 0.1f * elapsed;
					if (raiseDir == 0.0f && flattenHeight == 0.0f)
						flattenHeight = totalHeight / affectedVertices.size();
					for (auto v : affectedVertices) {
						auto& p = v->position;
						auto& n = v->normal;
						float vDistSq = p.x * p.x + p.y * p.y + p.z * p.z;
						float diff = flattenHeight != 0.0f ? sqrt(vDistSq) - flattenHeight : 0.0f;
						float raiseDir2 = raiseDir != 0.0f ? raiseDir : (diff > 0.0f ? -1.0f : 1.0f);
						float ratio = raiseDir2 >= 0.0f ? mostExtremeMin / vDistSq : vDistSq / mostExtremeMax;
						float dist2 = dist * raiseDir2 * ratio * ratio;
						float yaw = atan2(p.z, p.x);
						float pitch = atan2(-p.y, sqrtf(p.x * p.x + p.z * p.z)) + HALFPI;
						if (flattenHeight != 0.0f && abs(diff) < abs(dist2)) {
							SetHeight(yaw, pitch, flattenHeight);
							p.x = n.x * flattenHeight; p.y = n.y * flattenHeight; p.z = n.z * flattenHeight;
						}
						else {
							SetHeight(yaw, pitch, GetHeight(yaw, pitch) + dist2);
							p.x += n.x * dist2; p.y += n.y * dist2; p.z += n.z * dist2;
						}
					}
					world->meshes[0]->CreateBuffers(D3D11_CPU_ACCESS_WRITE);
				}
			}
			else {
				flattenHeight = 0.0f;
			}
		}
		else if (brushBuffer.data.cursorFlags & 1) {
			brushBuffer.data.cursorFlags = 0;
			brushBuffer.UpdateSubresource();
			//swprintf_s(message, L"");
		}

		// set the lightingBuffer information
		XMVECTOR positionVector = XMLoadFloat3(&Uber::I().camera->position);
		XMStoreFloat4(&lightingBuffer.data.viewPosition, positionVector);
		lightingBuffer.UpdateSubresource();

		// clear the back buffer with the background color and clear the depth buffer
		float color[4] = { 0.f, 0.f, 0.f, 1.f };
		context->ClearRenderTargetView(Uber::I().renderTargetView, color);
		context->ClearDepthStencilView(Uber::I().depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// first, do the depth map pass
		depthMap.BeginRender();
		depthMapShader->SwitchTo();
		for (auto* model : shadowCasters) {
			Transform& t = *model->transform;
			//worldMatrix = XMMatrixIdentity();
			auto& scaleVector = XMLoadFloat4(&XMFLOAT4(t.scaleX, t.scaleY, t.scaleZ, 0.0f));
			auto& translationVector = XMLoadFloat4(&XMFLOAT4(t.x, t.y, t.z, 1.0f));
			auto& rotationVector = XMLoadFloat4(&XMFLOAT4(t.pitch, t.yaw, t.roll, 0.0f));
			worldMatrix = XMMatrixScalingFromVector(scaleVector) *
				XMMatrixRotationRollPitchYawFromVector(rotationVector) *
				XMMatrixTranslationFromVector(translationVector);
			XMMATRIX worldViewProj = worldMatrix * XMLoadFloat4x4(&Uber::I().lightCamera->view) * XMLoadFloat4x4(&Uber::I().lightCamera->proj);
			depthMapBuffer.data.worldViewProj = XMMatrixTranspose(worldViewProj);

			for (auto* mesh : model->meshes) {
				for (auto& binding : mesh->depthMapTextureBindings) {
					switch (binding.shaderType) {
						case ShaderTypeVertex:
							depthMapBuffer.data.vsSlotsUsed |= 1 << binding.shaderSlot;
							break;
						default:
							break;
					}
				}

				depthMapBuffer.UpdateSubresource();
				mesh->DrawDepthMap();
			}
		}
		depthMap.EndRender();


		// render everything to the scene render target to allow post processing
		context->RSSetState(wireframe ? rasterStateWireframe : rasterStateSolid);
		sceneTarget.BeginRender();

		for (auto* model : models) {
			// add rotation to the sphere
			if (Uber::I().camera->focus && model == Uber::I().camera->focus) {
				XMVECTOR upVector = XMLoadFloat3(&worldUp);
				XMVECTOR rightVector = XMLoadFloat3(&XMFLOAT3(1.0f, 0.0f, 0.0f));
				worldMatrix = XMMatrixRotationAxis(upVector, Uber::I().camera->focusYaw) * XMMatrixRotationAxis(rightVector, -Uber::I().camera->focusPitch);
			}
			else {
				Transform& t = *model->transform;
				//worldMatrix = XMMatrixIdentity();
				auto& scaleVector = XMLoadFloat4(&XMFLOAT4(t.scaleX, t.scaleY, t.scaleZ, 0.0f));
				auto& translationVector = XMLoadFloat4(&XMFLOAT4(t.x, t.y, t.z, 1.0f));
				// align the rotation to the grid
				float gridX = 128.0f; // diffuseTexture->w;
				float gridY = 64.0f; // diffuseTexture->h;
				float yaw = floor(t.yaw * gridX) / gridX;
				float pitch = floor(t.pitch * gridY) / gridY;
				auto& rotationVector = XMLoadFloat4(&XMFLOAT4(pitch, yaw, t.roll, 0.0f));
				worldMatrix = XMMatrixScalingFromVector(scaleVector) *
					XMMatrixRotationRollPitchYawFromVector(rotationVector) *
					XMMatrixTranslationFromVector(translationVector);
			}
			// update the constant buffers that are constant for all meshes
			// shaders must receive transposed matrices in DirectX11
			matrixBuffer.data.world = XMMatrixTranspose(worldMatrix);
			matrixBuffer.data.view = XMMatrixTranspose(viewMatrix);
			matrixBuffer.data.projection = XMMatrixTranspose(projectionMatrix);
			matrixBuffer.UpdateSubresource();
			XMMATRIX lightViewMatrix = XMLoadFloat4x4(&Uber::I().lightCamera->view);
			XMMATRIX lightProjectionMatrix = XMLoadFloat4x4(&Uber::I().lightCamera->proj);
			// to do:  something's wrong with this
			lightingBuffer.data.shadowMatrix = XMMatrixTranspose(worldMatrix * lightViewMatrix * lightProjectionMatrix * XMLoadFloat4x4(&matNDCtoUV));
			lightingBuffer.data.shadowMapTexelSize = XMFLOAT2(1.0f / depthMap.width, 1.0f / depthMap.height);
			lightingBuffer.UpdateSubresource();

			materialBuffer.data.time = time();

			for (auto* mesh : model->meshes) {

				// render the model using the lit texture shader

				mesh->shader->SwitchTo();
				// hack in the shadow sampler
				if (mesh->shader == litTexture) {
					Uber::I().context->PSSetSamplers(1, 1, &shadowSamplerState);
				}

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
				materialBuffer.UpdateSubresource();

				mesh->Draw();
			}
		}

		// switch back to the back buffer
		sceneTarget.EndRender();
		context->RSSetState(rasterStateSolid);

		// post processing
		postProcessShader->SwitchTo();
		//sceneTarget.BindTexture(0);
		if (showDepthMap)
			depthMap.BindTexture(0);
		else
			sceneTarget.BindTexture(0);
		screen->meshes[0]->Draw();
		//sceneTarget.UnbindTexture(0);
		if (showDepthMap)
			depthMap.UnbindTexture(0);
		else
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
	depthMapShader->Release();
	if (shadowSamplerState) shadowSamplerState->Release();
	Uber::I().resourceManager->Terminate();
	delete Uber::I().resourceManager;
	delete Uber::I().camera;
	delete Uber::I().lightCamera;
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
	if (rasterStateSolid) rasterStateSolid->Release();
	if (rasterStateWireframe) rasterStateWireframe->Release();
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