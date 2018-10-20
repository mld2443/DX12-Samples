////////////////////////////////////////////////////////////////////////////////
// Filename: resourcesclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "resourcesclass.h"


ResourcesClass::ResourcesClass()
{
	m_swapChain = nullptr;

	m_d3d12Device = nullptr;
	m_commandQueue = nullptr;
	m_renderTargetViewHeap = nullptr;
	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_backBufferRenderTarget[i] = nullptr;
	}
	m_commandAllocator = nullptr;
	m_commandList = nullptr;
	m_pipelineState = nullptr;
	m_fence = nullptr;
	m_fenceEvent = nullptr;

	m_d3d11On12Device = nullptr;
	m_d3d11DeviceContext = nullptr;
	m_d2dDevice = nullptr;
	m_d2dDeviceContext = nullptr;
	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_wrappedBackBuffers[i] = nullptr;
		m_bitmaps[i] = nullptr;
	}

	m_dWriteFactory = nullptr;
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


	// Initialize all direct3D 12 resources.
	result = InitializeDirect3D12(screenHeight, screenWidth, hwnd, vsync, fullscreen);
	if (!result)
	{
		return false;
	}

	// Initialize all direct2D resources.
	result = InitializeDirect2D();
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct2D.", L"Initializer Failure", MB_OK);
		return false;
	}

	// Initialize all directWrite resources.
	result = InitializeDirectWrite();
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize DirectWrite.", L"Initializer Failure", MB_OK);
		return false;
	}

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

	ShutdownDirect3D12();

	// Release the swap chain.
	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	return;
}


bool ResourcesClass::BeginScene(float red, float green, float blue, float alpha)
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];


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
	barrier.Flags =						D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource =		m_backBufferRenderTarget[m_bufferIndex];
	barrier.Transition.StateBefore =	D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter =		D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource =	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type =						D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	// Get the render target view handle for the current back buffer.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize * m_bufferIndex;

	// Set the back buffer as the render target.
	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, nullptr);

	// Then set the color to clear the window to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;
	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, nullptr);

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

	return true;
}


void ResourcesClass::BeginDirect2D()
{
	// Acquire our wrapped render target resource for the current back buffer.
	m_d3d11On12Device->AcquireWrappedResources(&m_wrappedBackBuffers[m_bufferIndex], 1);

	// Render text directly to the back buffer.
	m_d2dDeviceContext->SetTarget(m_bitmaps[m_bufferIndex]);

	return;
}


void ResourcesClass::EndDirect2D()
{
	// Release our wrapped render target resource. Releasing 
	// transitions the back buffer resource to the state specified
	// as the OutState when the wrapped resource was created.
	m_d3d11On12Device->ReleaseWrappedResources(&m_wrappedBackBuffers[m_bufferIndex], 1);

	// Flush to submit the 11 command list to the shared command queue.
	m_d3d11DeviceContext->Flush();

	return;
}


bool ResourcesClass::EndScene()
{
	HRESULT result;
	unsigned long long fenceToWaitFor;


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

	// Update the back buffer index to point to the next frame.
	++m_bufferIndex;
	m_bufferIndex %= FRAME_BUFFER_COUNT;

	return true;
}


ID2D1Device4* ResourcesClass::GetDirect2DDevice()
{
	return m_d2dDevice;
}


ID2D1DeviceContext4* ResourcesClass::GetDirect2DDeviceContext()
{
	return m_d2dDeviceContext;
}


IDWriteFactory* ResourcesClass::GetDirectWriteFactory()
{
	return m_dWriteFactory;
}


bool ResourcesClass::InitializeDirect3D12(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
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
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
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
	result = D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&m_d3d12Device));
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not create a DirectX 12.1 device.  The default video card does not support DirectX 12.1.", L"DirectX Device Failure", MB_OK);
		return false;
	}

	// Initialize the description of the command queue.
	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

	// Set up the description of the command queue.
	commandQueueDesc.Type =		D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority =	D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags =	D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask =	0;

	// Create the command queue.
	result = m_d3d12Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue));
	if (FAILED(result))
	{
		return false;
	}

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
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
	swapChainDesc.BufferCount =			2;

	// Set the height and width of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Height =	screenHeight;
	swapChainDesc.BufferDesc.Width =	screenWidth;

	// Set a regular 32-bit surface for the back buffers.
	swapChainDesc.BufferDesc.Format =	DXGI_FORMAT_B8G8R8A8_UNORM;

	// Set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage =			DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the swap effect to discard the previous buffer contents after swapping.
	swapChainDesc.SwapEffect =			DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow =		hwnd;

	// Set to full screen or windowed mode.
	if (fullscreen)
	{
		swapChainDesc.Windowed =	false;
	}
	else
	{
		swapChainDesc.Windowed =	true;
	}

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator =	numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator =	denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator =	0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator =	1;
	}

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count =			1;
	swapChainDesc.SampleDesc.Quality =			0;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering =	DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling =			DXGI_MODE_SCALING_UNSPECIFIED;

	// Don't set the advanced flags.
	swapChainDesc.Flags =						0;

	// Finally create the swap chain using the swap chain description.	
	result = factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// Next upgrade the IDXGISwapChain to a IDXGISwapChain3 interface and store it in a private member variable named m_swapChain.
	// This will allow us to use the newer functionality such as getting the current back buffer index.
	result = swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain));
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

	// Set the number of descriptors to two for our back buffers.  Also set the heap type to render target views.
	renderTargetViewHeapDesc.NumDescriptors =	FRAME_BUFFER_COUNT;
	renderTargetViewHeapDesc.Type =				D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags =			D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the render target view heap for the back buffers.
	result = m_d3d12Device->CreateDescriptorHeap(&renderTargetViewHeapDesc, IID_PPV_ARGS(&m_renderTargetViewHeap));
	if (FAILED(result))
	{
		return false;
	}

	// Get a handle to the starting memory location in the render target view heap to identify where the render target views will be located for the two back buffers.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// Get the size of the memory location for the render target view descriptors.
	renderTargetViewDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		// Get a pointer to the current back buffer from the swap chain.
		result = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBufferRenderTarget[i]));
		if (FAILED(result))
		{
			return false;
		}

		// Create a render target view for the first back buffer.
		m_d3d12Device->CreateRenderTargetView(m_backBufferRenderTarget[i], nullptr, renderTargetViewHandle);

		// Increment the view handle to the next descriptor location in the render target view heap.
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	// Finally get the initial index to which buffer is the current back buffer.
	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create a command allocator.
	result = m_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(result))
	{
		return false;
	}

	// Create a basic command list.
	result = m_d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&m_commandList));
	if (FAILED(result))
	{
		return false;
	}

	// Initially we need to close the command list during initialization as it is created in a recording state.
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	// Create a fence for GPU synchronization.
	result = m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(result))
	{
		return false;
	}

	// Create an event object for the fence.
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL)
	{
		return false;
	}

	// Initialize the starting fence value. 
	m_fenceValue = 1;

	return true;
}


bool ResourcesClass::InitializeDirect2D()
{
	HRESULT result;
	UINT d3d11DeviceFlags;
	D2D1_FACTORY_OPTIONS options;
	ID2D1Factory5* d2dFactory;
	ID3D11Device *d3d11Device;
	IDXGIDevice* dxgiDevice;
	IDXGISurface* dxgiSurface;
	float dpiX, dpiY;
	D2D1_BITMAP_PROPERTIES1 bitmapProperties;
	D3D11_RESOURCE_FLAGS d3d11ResourceFlags;


	// Set the direct3d 11 flag, necessary for d2d compatibility.
	d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	// Set the default options for the D2DFactory.
	ZeroMemory(&options, sizeof(options));

#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers.
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

	// Also enable d3d11 debugging.
	d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create D2DFactory to get our device.
	result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &d2dFactory);
	if (FAILED(result))
	{
		return false;
	}

	// Create a D3D 11 device from our D3D 12 device and its command queue.
	result = D3D11On12CreateDevice(m_d3d12Device, d3d11DeviceFlags, nullptr, 0, (IUnknown**)&m_commandQueue,
									1, 0, &d3d11Device, &m_d3d11DeviceContext, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	// Create a D3D11On12 device wrapper of our D3D 11 Device.
	result = d3d11Device->QueryInterface(IID_PPV_ARGS(&m_d3d11On12Device));
	if (FAILED(result))
	{
		return false;
	}

	// Retrieve a pointer to our device as a DXGI device.
	result = m_d3d11On12Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
	if (FAILED(result))
	{
		return false;
	}

	// Store a reference to our device.
	result = d2dFactory->CreateDevice(dxgiDevice, &m_d2dDevice);
	if (FAILED(result))
	{
		return false;
	}

	// Retrieve the device context DPI to match the text DPI with.
	d2dFactory->GetDesktopDpi(&dpiX, &dpiY);

	// Release the D3D11 device.
	d3d11Device = nullptr;

	// Release the factory.
	d2dFactory->Release();
	d2dFactory = nullptr;

	// Release the DXGI device.
	dxgiDevice->Release();
	dxgiDevice = nullptr;

	// Create a device context to draw with.
	result = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dDeviceContext);
	if (FAILED(result))
	{
		return false;
	}

	// Clear the memory of our pixel format.
	ZeroMemory(&bitmapProperties, sizeof(bitmapProperties));

	// Set the properties of our bitmap.
	bitmapProperties.bitmapOptions =			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	bitmapProperties.pixelFormat.format =		DXGI_FORMAT_B8G8R8A8_UNORM;
	bitmapProperties.pixelFormat.alphaMode =	D2D1_ALPHA_MODE_IGNORE;
	bitmapProperties.dpiX =						dpiX;
	bitmapProperties.dpiY =						dpiY;
	bitmapProperties.colorContext =				NULL;

	// Clear the memory of our pixel format.
	ZeroMemory(&d3d11ResourceFlags, sizeof(d3d11ResourceFlags));

	// Set the properties of our bitmap.
	d3d11ResourceFlags.BindFlags = D3D11_BIND_RENDER_TARGET;

	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		// Create a wrapped 11On12 resource of this back buffer. Since we are 
		// rendering all D3D12 content first and then all D2D content, we specify 
		// the In resource state as RENDER_TARGET - because D3D12 will have last 
		// used it in this state - and the Out resource state as PRESENT. When 
		// ReleaseWrappedResources() is called on the 11On12 device, the resource 
		// will be transitioned to the PRESENT state.
		result = m_d3d11On12Device->CreateWrappedResource(m_backBufferRenderTarget[i], &d3d11ResourceFlags,
														D3D12_RESOURCE_STATE_RENDER_TARGET,
														D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(&m_wrappedBackBuffers[i]));
		if (FAILED(result))
		{
			return false;
		}

		// Create a render target for D2D to draw directly to this back buffer.
		result = m_wrappedBackBuffers[i]->QueryInterface(IID_PPV_ARGS(&dxgiSurface));
		if (FAILED(result))
		{
			return false;
		}

		// Use that render target to create a bitmap onto which the D2D can draw directly.
		result = m_d2dDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface, &bitmapProperties, &m_bitmaps[i]);
		if (FAILED(result))
		{
			return false;
		}

		// Release the DXGI surface device.
		dxgiSurface->Release();
		dxgiSurface = nullptr;
	}

	return true;
}


bool ResourcesClass::InitializeDirectWrite()
{
	HRESULT result;


	// Create a DirectWrite Factory to create layouts and formats with.
	result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory4), (IUnknown **)&m_dWriteFactory);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}


void ResourcesClass::ShutdownDirect3D12()
{
	int error;


	// Close the object handle to the fence event.
	error = CloseHandle(m_fenceEvent);
	if (error == 0)
	{
	}

	// Release the fence.
	if (m_fence)
	{
		m_fence->Release();
		m_fence = nullptr;
	}

	// Release the empty pipe line state.
	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = nullptr;
	}

	// Release the command list.
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}

	// Release the command allocator.
	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = nullptr;
	}

	// Release the back buffer render target views.
	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; ++i)
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

	// Release the command queue.
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = nullptr;
	}

	// Release the d3d12 device.
	if (m_d3d12Device)
	{
		m_d3d12Device->Release();
		m_d3d12Device = nullptr;
	}

	return;
}


void ResourcesClass::ShutdownDirect2D()
{
	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		// Release the bitmap drawing surfaces.
		if (m_bitmaps)
		{
			m_bitmaps[i]->Release();
			m_bitmaps[i] = nullptr;
		}

		// Release the d3d11 wrapped back buffers.
		if (m_wrappedBackBuffers[i])
		{
			m_wrappedBackBuffers[i]->Release();
			m_wrappedBackBuffers[i] = nullptr;
		}
	}

	// Release the d2d device context.
	if (m_d2dDeviceContext)
	{
		m_d2dDeviceContext->Release();
		m_d2dDeviceContext = nullptr;
	}

	// Release the d2d device.
	if (m_d2dDevice)
	{
		m_d2dDevice->Release();
		m_d2dDevice = nullptr;
	}

	// Release the d3d11 device context.
	if (m_d3d11DeviceContext)
	{
		m_d3d11DeviceContext->Release();
		m_d3d11DeviceContext = nullptr;
	}

	// Release the d3d11 wrapper device.
	if (m_d3d11On12Device)
	{
		m_d3d11On12Device->Release();
		m_d3d11On12Device = nullptr;
	}

	return;
}


void ResourcesClass::ShutdownDirectWrite()
{
	if (m_dWriteFactory)
	{
		m_dWriteFactory->Release();
		m_dWriteFactory = nullptr;
	}

	return;
}
