// include the basic windows header file
#include <windows.h>
#include <windowsx.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <sstream>
#include <tuple>
using namespace DirectX;
using namespace std;

struct MatrixBufferType {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};

struct VertexType {
	XMFLOAT3 position;
	XMFLOAT4 color;
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

	int screenWidth = windowed ? 500 : GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = windowed ? 400 : GetSystemMetrics(SM_CYSCREEN);
	RECT wr = {0, 0, screenWidth, screenHeight};    // set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

	// create the window and use the result as the handle
	hWnd = CreateWindowEx(NULL,
		L"WindowClass1",    // name of the window class
		L"Our First Windowed Program",   // title of the window
		WS_OVERLAPPEDWINDOW,    // window style
		300,    // x-position of the window
		300,    // y-position of the window
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
	// initialize the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1; // single back buffer
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	// turn multisampling off
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = windowed;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// discard the back buffer contents after presenting
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

	// find the video memory in megabytes
	DXGI_ADAPTER_DESC adapterDesc;
	adapter->GetDesc(&adapterDesc);
	size_t videoMemoryMb = adapterDesc.DedicatedVideoMemory / 1024 / 1024;

	// get the video card name
	char videoCardName[128];
	size_t stringLength;
	wcstombs_s(&stringLength, videoCardName, 128, adapterDesc.Description, 128);

	adapterOutput->Release();
	adapter->Release();
	factory->Release();

	// create the swap chain, device, context, and rendertargetview
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1};
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, NULL, &context);
	ID3D11Texture2D* backBuffer = 0;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	ID3D11RenderTargetView* renderTargetView;
	device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
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

	// bind the render target view and depth stencil buffer to the output render pipeline
	context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

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

	// create the projection matrix
	float fieldOfView = 3.141592654f / 4.0f;
	float screenAspect = (float)screenWidth / (float)screenHeight;
	float screenDepth = 1000.0f;
	float screenNear = 0.1f;
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	// create the world matrix
	XMMATRIX worldMatrix = XMMatrixIdentity();

	// create the view matrix
	// todo

	// create an orthographic projection matrix for 2D UI rendering.
	XMMATRIX orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	// shaders
	HRESULT result;
	ID3D10Blob* errorMessage = 0;
	ID3D10Blob* vertexShaderBuffer = 0;
	ID3D10Blob* pixelShaderBuffer = 0;
	// compile the vertex and pixel shaders
	//assert(!FAILED(result = D3DCompileFromFile(L"color.vs", NULL, NULL, "ColorVertexShader", "vs_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage)));
	//assert(!FAILED(result = D3DCompileFromFile(L"color.ps", NULL, NULL, "ColorPixelShader", "ps_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage)));
	for (auto shaderInfo : {
		tuple<LPCWSTR, char*, char*, ID3D10Blob**>(L"color.vs", "ColorVertexShader", "vs_4_0", &vertexShaderBuffer),
		tuple<LPCWSTR, char*, char*, ID3D10Blob**>(L"color.ps", "ColorPixelShader", "ps_4_0", &pixelShaderBuffer)
	}) {
		result = D3DCompileFromFile(get<0>(shaderInfo), NULL, NULL, get<1>(shaderInfo), get<2>(shaderInfo), D3D10_SHADER_ENABLE_STRICTNESS, 0, get<3>(shaderInfo), &errorMessage);
		if (FAILED(result)) {
			if (errorMessage) {
				// show a message box with the compile errors
				char* compileErrors = (char*)(errorMessage->GetBufferPointer());
				wstringstream msg;
				for (size_t i = 0; i < errorMessage->GetBufferSize(); i++)
					msg << compileErrors[i];
				errorMessage->Release();
				MessageBox(hWnd, msg.str().c_str(), get<0>(shaderInfo), MB_OK);
			}
			else {
				MessageBox(hWnd, get<0>(shaderInfo), L"Missing Shader File", MB_OK);
			}
			break;
		}
	}
	ID3D11VertexShader* vertexShader = 0;
	ID3D11PixelShader* pixelShader = 0;
	assert(!FAILED(result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader)));
	assert(!FAILED(result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader)));
	
	// create the vertex input layout description
	// this setup needs to match the VertexType stucture in the shader
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	size_t numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
	ID3D11InputLayout* layout;
	assert(!FAILED(result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &layout)));
	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	// dynamic matrix constant buffer that's in the vertex shader
	ID3D11Buffer* matrixBuffer = 0;
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	assert(!FAILED(result = device->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer)));


	// make a triangle
	unsigned vertexCount = 3;
	unsigned indexCount = 3;
	VertexType* vertices = new VertexType[vertexCount];
	unsigned long* indices = new unsigned long[indexCount];
	vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // bottom left
	vertices[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // top middle
	vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // bottom right
	vertices[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	indices[0] = 0;  // bottom left
	indices[1] = 1;  // top middle
	indices[2] = 2;  // bottom right

	// create the vertex and index buffers
	ID3D11Buffer *vertexBuffer, *indexBuffer;
	for (auto bufferInfo : {
		tuple<unsigned, void*, ID3D11Buffer**>(sizeof(VertexType) * vertexCount, vertices, &vertexBuffer),
		tuple<unsigned, void*, ID3D11Buffer**>(sizeof(unsigned long) * indexCount, indices, &indexBuffer)
	}) {
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = get<0>(bufferInfo);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		// give the subresource structure a pointer to the raw data
		D3D11_SUBRESOURCE_DATA bufferData;
		bufferData.pSysMem = get<1>(bufferInfo);
		bufferData.SysMemPitch = 0;
		bufferData.SysMemSlicePitch = 0;

		// create the buffer
		assert(!FAILED(result = device->CreateBuffer(&bufferDesc, &bufferData, get<2>(bufferInfo))));
	}
	delete[] vertices;
	delete[] indices;


	// set up the camera
	XMFLOAT3 position(0.f, 0.f, -5.f);
	XMFLOAT3 rotation(0.f, 0.f, 0.f);
	XMFLOAT3 up(0.f, 1.f, 0.f);
	XMFLOAT3 forward(0.f, 0.f, 1.f);

	// main loop
	MSG msg = {0};
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
		} else {
			// clear the back buffer with the background color and clear the depth buffer
			float color[4] = {1.f, 0.f, 0.f, 1.f};
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


			// stage the triangle's buffers as the ones to use
			// set the vertex buffer to active in the input assembler
			unsigned stride = sizeof(VertexType);
			unsigned offset = 0;
			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			// set the index buffer to active in the input assembler
			context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


			// render the triangle using the color shader

			// lock the matrixBuffer, set the new matrices inside it, and then unlock it
			// shaders must receive transposed matrices in DirectX11
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			assert(!FAILED(result = context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)));
			MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;
			dataPtr->world = XMMatrixTranspose(worldMatrix);
			dataPtr->view = XMMatrixTranspose(viewMatrix);
			dataPtr->projection = XMMatrixTranspose(projectionMatrix);
			context->Unmap(matrixBuffer, 0);

			// update the vertex shader constant buffer and set the position of the constant buffer
			unsigned bufferNumber = 0;
			context->VSSetConstantBuffers(bufferNumber, 1, &matrixBuffer);

			// set the vertex input layout
			context->IASetInputLayout(layout);

			// set the vertex and pixel shaders that will be used to render this triangle
			context->VSSetShader(vertexShader, NULL, 0);
			context->PSSetShader(pixelShader, NULL, 0);

			// render the triangle
			context->DrawIndexed(indexCount, 0, 0);


			// Present the back buffer to the screen since rendering is complete.
			swapChain->Present(vsync ? 1 : 0, 0);
		}
	}

	// clean up
	if (swapChain && !windowed) swapChain->SetFullscreenState(false, NULL);
	if (vertexBuffer) vertexBuffer->Release();
	if (indexBuffer) indexBuffer->Release();
	if (matrixBuffer) matrixBuffer->Release();
	if (layout) layout->Release();
	if (vertexShader) vertexShader->Release();
	if (pixelShader) pixelShader->Release();
	if (rasterState) rasterState->Release();
	if (depthStencilView) depthStencilView->Release();
	if (depthStencilState) depthStencilState->Release();
	if (depthStencilBuffer) depthStencilBuffer->Release();
	if (renderTargetView) renderTargetView->Release();
	if (context) context->Release();
	if (device) device->Release();
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