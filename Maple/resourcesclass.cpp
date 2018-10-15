////////////////////////////////////////////////////////////////////////////////
// Filename: resourcesclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "resourcesclass.h"


ResourcesClass::ResourcesClass()
{
	m_swapChain = nullptr;

	m_direct3DDevice = nullptr;
	m_commandQueue = nullptr;
	m_renderTargetViewHeap = nullptr;

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_backBufferRenderTarget[i] = nullptr;
	}

	m_direct2DDevice = nullptr;
	m_direct2DDeviceContext = nullptr;
	m_bitmap = nullptr;

	m_directWriteFactory = nullptr;
}


ResourcesClass::ResourcesClass(const ResourcesClass& other)
{
}


ResourcesClass::~ResourcesClass()
{
}


bool ResourcesClass::Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
{
	bool result;


	// Initialize all direct3D resources.
	result = InitializeDirect3D(screenHeight, screenWidth, hwnd, vsync, fullscreen);
	if (!result)
	{
		return false;
	}

	// Initialize all direct2D resources.
	//result = InitializeDirect2D();
	//if (!result)
	//{
	//	return false;
	//}

	// Initialize all directWrite resources.
	//result = InitializeDirectWrite();
	//if (!result)
	//{
	//	return false;
	//}

	return true;
}


void ResourcesClass::Shutdown()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, nullptr);
	}

	ShutdownDirectWrite();

	ShutdownDirect2D();

	ShutdownDirect3D();

	return;
}


bool ResourcesClass::Render(ID3D12CommandList** commandLists, UINT numCommandLists)
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];
	unsigned long long fenceToWaitFor;


	// Reset (re-use) the memory associated command allocator.
	result = m_commandAllocator->Reset();
	if (FAILED(result))
	{
		return false;
	}

	// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
	result = m_commandList->Reset(m_commandAllocator, m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}

	// Record commands in the command list now.
	// Start by setting the resource barrier.
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBufferRenderTarget[m_bufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	// Get the render target view handle for the current back buffer.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_direct3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	renderTargetViewHandle.ptr += (renderTargetViewDescriptorSize * m_bufferIndex);

	// Set the back buffer as the render target.
	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, nullptr);

	// Then set the color to clear the window to.
	color[0] = 0.0;
	color[1] = 0.5;
	color[2] = 0.0;
	color[3] = 1.0;
	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, nullptr);

	// Indicate that the back buffer will now be used to present.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	// Close the list of commands.
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	// Load the command list array (only one command list for now).
	ppCommandLists[0] = m_commandList;

	// Execute the list of commands.
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

	// Finally present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		result = m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		result = m_swapChain->Present(0, 0);
	}
	if (FAILED(result))
	{
		return false;
	}

	// Signal and increment the fence value.
	fenceToWaitFor = m_fenceValue;
	result = m_commandQueue->Signal(m_fence, fenceToWaitFor);
	if (FAILED(result))
	{
		return false;
	}
	m_fenceValue++;

	// Wait until the GPU is done rendering.
	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
		if (FAILED(result))
		{
			return false;
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// Alternate the back buffer index back and forth between 0 and 1 each frame.
	m_bufferIndex++;
	m_bufferIndex %= FRAME_BUFFER_COUNT;

	return true;
}


unsigned int ResourcesClass::GetBufferIndex()
{
	return m_bufferIndex;
}


bool ResourcesClass::InitializeDirect3D(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
{
	HRESULT result;
	ID3D12Debug* debugController;
	D3D_FEATURE_LEVEL featureLevel;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	IDXGIFactory4* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator, renderTargetViewDescriptorSize;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	IDXGISwapChain* swapChain;
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;


	// Store the vsync setting.
	m_vsync_enabled = vsync;

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	featureLevel = D3D_FEATURE_LEVEL_12_1;

#if defined(_DEBUG)
	// Create the Direct3D debug controller.
	result = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debugController);
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not enable Direct3D debugging.", L"Debugger Failure", MB_OK);
		return false;
	}

	// Enable the debug layer.
	debugController->EnableDebugLayer();

	// Release the debug controller.
	debugController->Release();
	debugController = nullptr;
#endif

	// Create the Direct3D 12 device.
	result = D3D12CreateDevice(nullptr, featureLevel, __uuidof(ID3D12Device), (void**)&m_direct3DDevice);
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not create a DirectX 12.1 device.  The default video card does not support DirectX 12.1.", L"DirectX Device Failure", MB_OK);
		return false;
	}

	// Initialize the description of the command queue.
	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

	// Set up the description of the command queue.
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	// Create the command queue.
	result = m_direct3DDevice->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_commandQueue);
	if (FAILED(result))
	{
		return false;
	}

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_B8G8R8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen height and width.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Height == (unsigned int)screenHeight)
		{
			if (displayModeList[i].Width == (unsigned int)screenWidth)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = nullptr;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = nullptr;

	// Release the adapter.
	adapter->Release();
	adapter = nullptr;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set the swap chain to use double buffering.
	swapChainDesc.BufferCount = 2;

	// Set the height and width of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Width = screenWidth;

	// Set a regular 32-bit surface for the back buffers.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	// Set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the swap effect to discard the previous buffer contents after swapping.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Set to full screen or windowed mode.
	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Finally create the swap chain using the swap chain description.	
	result = factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// Next upgrade the IDXGISwapChain to a IDXGISwapChain3 interface and store it in a private member variable named m_swapChain.
	// This will allow us to use the newer functionality such as getting the current back buffer index.
	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// Clear pointer to original swap chain interface since we are using version 3 instead (m_swapChain).
	swapChain = nullptr;

	// Release the factory now that the swap chain has been created.
	factory->Release();
	factory = nullptr;

	// Initialize the render target view heap description for the two back buffers.
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));

	// Set the number of descriptors to two for our two back buffers.  Also set the heap tyupe to render target views.
	renderTargetViewHeapDesc.NumDescriptors = 2;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the render target view heap for the back buffers.
	result = m_direct3DDevice->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if (FAILED(result))
	{
		return false;
	}

	// Get a handle to the starting memory location in the render target view heap to identify where the render target views will be located for the two back buffers.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// Get the size of the memory location for the render target view descriptors.
	renderTargetViewDescriptorSize = m_direct3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a pointer to the first back buffer from the swap chain.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if (FAILED(result))
	{
		return false;
	}

	// Create a render target view for the first back buffer.
	m_direct3DDevice->CreateRenderTargetView(m_backBufferRenderTarget[0], nullptr, renderTargetViewHandle);

	// Increment the view handle to the next descriptor location in the render target view heap.
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// Get a pointer to the second back buffer from the swap chain.
	result = m_swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if (FAILED(result))
	{
		return false;
	}

	// Create a render target view for the second back buffer.
	m_direct3DDevice->CreateRenderTargetView(m_backBufferRenderTarget[1], nullptr, renderTargetViewHandle);

	// Finally get the initial index to which buffer is the current back buffer.
	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	return true;
}


bool ResourcesClass::InitializeDirect2D()
{
	HRESULT result;
	FLOAT dpiX, dpiY;
	D2D1_FACTORY_OPTIONS options;
	ID2D1Factory5* factory;
	IDXGISurface* dxgiSurface;
	IDXGIDevice* dxgiDevice;
	D2D1_BITMAP_PROPERTIES1 bitmapProperties;
	D2D1_PIXEL_FORMAT pixelFormat;


	// Set the default options for the D2DFactory.
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers.
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	// Create D2DFactory to get our device.
	result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &factory);
	if (FAILED(result))
	{
		return false;
	}

	// Retrieve a pointer to our device as a DXGI device.
	result = m_direct3DDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
	if (FAILED(result))
	{
		return false;
	}

	// Store a reference to our device.
	result = factory->CreateDevice(dxgiDevice, &m_direct2DDevice);
	if (FAILED(result))
	{
		return false;
	}

	// Release the DXGI device.
	dxgiDevice->Release();
	dxgiDevice = nullptr;

	// Create a device context to draw with.
	result = m_direct2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_direct2DDeviceContext);
	if (FAILED(result))
	{
		return false;
	}

	// Get the back buffer from the swap chain to use as a DXGI Surface.
	result = m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&dxgiSurface);
	if (FAILED(result))
	{
		return false;
	}

	// Retrieve the device context DPI to match the text DPI with.
	factory->GetDesktopDpi(&dpiX, &dpiY);

	// Release the factory.
	factory->Release();
	factory = nullptr;

	// Define Properties for our bitmap object.
	pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
	bitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, pixelFormat, dpiX, dpiY);

	// Create a bitmap to draw text on using the DXGI Surface backbuffer.
	result = m_direct2DDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface, &bitmapProperties, &m_bitmap);
	if (FAILED(result))
	{
		return false;
	}

	// Release the DXGI surface device.
	dxgiSurface->Release();
	dxgiSurface = nullptr;

	// Set our bitmap as the drawing target for our device context.
	m_direct2DDeviceContext->SetTarget(m_bitmap);

	return true;
}


bool ResourcesClass::InitializeDirectWrite()
{
	HRESULT result;


	// Create a DirectWrite Factory to create layouts and formats with.
	result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4), (IUnknown **)&m_directWriteFactory);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}


void ResourcesClass::ShutdownDirect3D()
{
	// Release the back buffer render target views.
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		if (m_backBufferRenderTarget[i])
		{
			m_backBufferRenderTarget[i]->Release();
			m_backBufferRenderTarget[i] = nullptr;
		}
	}

	// Release the render target view heap.
	if (m_renderTargetViewHeap)
	{
		m_renderTargetViewHeap->Release();
		m_renderTargetViewHeap = nullptr;
	}

	// Release the swap chain.
	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	// Release the command queue.
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = nullptr;
	}

	// Release the device.
	if (m_direct3DDevice)
	{
		m_direct3DDevice->Release();
		m_direct3DDevice = nullptr;
	}

	return;
}


void ResourcesClass::ShutdownDirect2D()
{
	if (m_bitmap)
	{
		m_bitmap->Release();
		m_bitmap = nullptr;
	}

	if (m_direct2DDeviceContext)
	{
		m_direct2DDeviceContext->Release();
		m_direct2DDeviceContext = nullptr;
	}

	if (m_direct2DDevice)
	{
		m_direct2DDevice->Release();
		m_direct2DDevice = nullptr;
	}

	return;
}


void ResourcesClass::ShutdownDirectWrite()
{
	if (m_directWriteFactory)
	{
		m_directWriteFactory->Release();
		m_directWriteFactory = nullptr;
	}

	return;
}
