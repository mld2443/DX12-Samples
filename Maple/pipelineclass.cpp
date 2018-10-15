////////////////////////////////////////////////////////////////////////////////
// Filename: colorshaderclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "pipelineclass.h"


PipelineClass::PipelineClass()
{
	m_rootSignature = nullptr;
	m_pipelineState = nullptr;

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_commandAllocator[i] = nullptr;
		m_fence[i] = nullptr;
	}
	m_commandList = nullptr;
	m_fenceEvent = NULL;
}


PipelineClass::PipelineClass(const PipelineClass& other)
{
}


PipelineClass::~PipelineClass()
{
}


bool PipelineClass::Initialize(ID3D12Device* device, unsigned int frameIndex)
{
	bool result;


	// Initialize the pipeline state.
	result = InitializePipeline(device);
	if (!result)
	{
		return false;
	}

	// Initialize the command list and its allocators.
	result = InitializeCommandList(device, frameIndex);
	if (!result)
	{
		return false;
	}

	return true;
}


void PipelineClass::Shutdown()
{
	// Shutdown the pipeline and its associated objects.
	ShutdownPipeline();

	// Shutdown the command list and its associated objects.
	ShutdownCommandList();

	return;
}


bool PipelineClass::BeginPipeline(XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix)
{
	bool result;


	// Set the shader parameters that it will use for rendering.
	result = SetMatrixBuffer(worldMatrix, viewMatrix, projectionMatrix);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	//RenderShader(deviceContext, indexCount);

	return true;
}


bool PipelineClass::InitializePipeline(ID3D12Device* device)
{
	HRESULT result;
	D3D12_ROOT_PARAMETER matrixBufferDesc;
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	ID3D10Blob *signature;
	D3D12_BLEND_DESC blendStateDesc;
	D3D12_RASTERIZER_DESC rasterDesc;
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	unsigned int numElements;
	D3D12_INPUT_ELEMENT_DESC polygonLayout[2];
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;


	//
	ZeroMemory(&matrixBufferDesc, sizeof(matrixBufferDesc));

	//
	matrixBufferDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	matrixBufferDesc.Descriptor.ShaderRegister = 0;
	matrixBufferDesc.Descriptor.RegisterSpace = 0;
	matrixBufferDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Specify which shaders need access to what resources.
	rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
						| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
						| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
						| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
						| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// Clear the root signature description before setting the parameters.
	ZeroMemory(&rootSignatureDesc, sizeof(rootSignatureDesc));

	// Fill out the root signature layout description.
	rootSignatureDesc.NumParameters =		1;
	rootSignatureDesc.pParameters =			&matrixBufferDesc;
	rootSignatureDesc.NumStaticSamplers =	0;
	rootSignatureDesc.pStaticSamplers =		nullptr;
	rootSignatureDesc.Flags =				rootSignatureFlags;

	// Serialize the signature, preparing it for creation on the device.
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	// Create the root signature on our device.
	result = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(result))
	{
		return false;
	}

	// Clear the blend state description.
	ZeroMemory(&blendStateDesc, sizeof(D3D12_BLEND_DESC));

	// Create an alpha enabled blend state description.
	blendStateDesc.AlphaToCoverageEnable =	FALSE;
	blendStateDesc.IndependentBlendEnable =	FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable =			TRUE;
	blendStateDesc.RenderTarget[0].LogicOpEnable =			FALSE;
	blendStateDesc.RenderTarget[0].SrcBlend =				D3D12_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend =				D3D12_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp =				D3D12_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha =			D3D12_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlendAlpha =			D3D12_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha =			D3D12_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask =	0x0f;

	// Clear the rasterizer state description. 
	ZeroMemory(&rasterDesc, sizeof(rasterDesc));

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.FillMode =				D3D12_FILL_MODE_SOLID;
	rasterDesc.CullMode =				D3D12_CULL_MODE_BACK;
	rasterDesc.FrontCounterClockwise =	false;
	rasterDesc.DepthBias =				0;
	rasterDesc.DepthBiasClamp =			0.0f;
	rasterDesc.SlopeScaledDepthBias =	0.0f;
	rasterDesc.DepthClipEnable =		true;
	rasterDesc.MultisampleEnable =		false;
	rasterDesc.AntialiasedLineEnable =	false;
	rasterDesc.ForcedSampleCount =		0;
	rasterDesc.ConservativeRaster =		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable =		true;
	depthStencilDesc.DepthWriteMask =	D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc =		D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthStencilDesc.StencilEnable =	true;
	depthStencilDesc.StencilReadMask =	0xFF;
	depthStencilDesc.StencilWriteMask =	0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp =		D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp =	D3D12_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp =		D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc =		D3D12_COMPARISON_FUNC_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp =		D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp =	D3D12_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp =		D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc =			D3D12_COMPARISON_FUNC_ALWAYS;

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName =			"POSITION";
	polygonLayout[0].SemanticIndex =		0;
	polygonLayout[0].Format =				DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot =			0;
	polygonLayout[0].AlignedByteOffset =	0;
	polygonLayout[0].InputSlotClass =		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate =	0;

	polygonLayout[1].SemanticName =			"COLOR";
	polygonLayout[1].SemanticIndex =		0;
	polygonLayout[1].Format =				DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[1].InputSlot =			0;
	polygonLayout[1].AlignedByteOffset =	D3D12_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass =		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate =	0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Clear the pipeline state description before setting the parameters.
	ZeroMemory(&pipelineStateDesc, sizeof(pipelineStateDesc));

	// Set up the Pipeline State for this render pipeline.
	pipelineStateDesc.pRootSignature =					m_rootSignature;
	pipelineStateDesc.VS.pShaderBytecode =				g_colorvs;
	pipelineStateDesc.VS.BytecodeLength =				sizeof(g_colorvs);
	pipelineStateDesc.PS.pShaderBytecode =				g_colorps;
	pipelineStateDesc.PS.BytecodeLength =				sizeof(g_colorps);
	pipelineStateDesc.BlendState =						blendStateDesc;
	pipelineStateDesc.SampleMask =						0xffffffff;
	pipelineStateDesc.RasterizerState =					rasterDesc;
	pipelineStateDesc.DepthStencilState =				depthStencilDesc;
	pipelineStateDesc.InputLayout.NumElements =			numElements;
	pipelineStateDesc.InputLayout.pInputElementDescs =	polygonLayout;
	pipelineStateDesc.PrimitiveTopologyType =			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.NumRenderTargets =				1;
	pipelineStateDesc.RTVFormats[0] =					DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.SampleDesc.Count =				1;
	pipelineStateDesc.SampleDesc.Quality =				0;
	pipelineStateDesc.Flags =							D3D12_PIPELINE_STATE_FLAG_NONE;

	// Create the pipeline state.
	result = device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(result))
	{
		return false;
	}

	//TODO: Set up vertex (and index?) buffers for the shader.


	//TODO: Set up view matrix communication with the shader here too.
	//TODO: Set up stencil and raster views and stuff here.

	return true;
}


bool PipelineClass::InitializeCommandList(ID3D12Device* device, unsigned int frameIndex)
{
	HRESULT result;


	// Create Frame dependant resources.
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		// Create command allocators, one for each frame.
		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]));
		if (FAILED(result))
		{
			return false;
		}

		// Create fences for GPU synchronization.
		result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i]));
		if (FAILED(result))
		{
			return false;
		}

		// Initialize the starting fence values. 
		m_fenceValue[i] = 1;
	}

	// Create a command list.
	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[frameIndex], nullptr, IID_PPV_ARGS(&m_commandList));
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

	// Create an event object for the fence.
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL)
	{
		return false;
	}

	return true;
}


void PipelineClass::ShutdownPipeline()
{
	// Release the pipeline state.
	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = nullptr;
	}

	// Release the root signature.
	if (m_rootSignature)
	{
		m_rootSignature->Release();
		m_rootSignature = nullptr;
	}

	return;
}


void PipelineClass::ShutdownCommandList()
{
	int error;


	// Close the object handle to the fence event.
	error = CloseHandle(m_fenceEvent);
	if (error == 0)
	{
	}

	// Release the command list itself.
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}

	// Release the frame dependant resources.
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		// Release the fences.
		if (m_fence[i])
		{
			m_fence[i]->Release();
			m_fence[i] = nullptr;
		}

		// Release the command allocators.
		if (m_commandAllocator[i])
		{
			m_commandAllocator[i]->Release();
			m_commandAllocator[i] = nullptr;
		}
	}

	return;
}


bool PipelineClass::AllocateBuffers(ID3D12Device* device, UINT64 vertexBufferSize, UINT64 indexBufferSize, UINT64 matrixBufferSize)
{
	D3D12_HEAP_PROPERTIES heapProps;
	D3D12_RESOURCE_DESC resourceDesc;
	HRESULT result;


	// Clear the heap properties before setting their values.
	ZeroMemory(&heapProps, sizeof(heapProps));
	
	// Specify the heap values. We will be recycling this structure.
	heapProps.Type =					D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty =			D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference =	D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask =		1;
	heapProps.VisibleNodeMask =			1;

	// Clear the resource description before setting its values.
	ZeroMemory(&resourceDesc, sizeof(resourceDesc));

	// Set up the resource buffer description. We will also be recycling this.
	resourceDesc.Dimension =			D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment =			0;
	resourceDesc.Width =				vertexBufferSize;
	resourceDesc.Height =				1;
	resourceDesc.DepthOrArraySize =		1;
	resourceDesc.MipLevels =			1;
	resourceDesc.Format =				DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count =		1;
	resourceDesc.SampleDesc.Quality =	0;
	resourceDesc.Layout =				D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags =				D3D12_RESOURCE_FLAG_NONE;

	// Allocate the upload heap for the vertex buffer on the GPU.
	// This is the buffer that our CPU has access to write to.
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
											IID_PPV_ARGS(&vertexUpload));
	if (FAILED(result))
	{
		return false;
	}

	// Set the resource name for debugging.
	vertexUpload->SetName(L"Vertex Buffer Upload Heap");

	// Set the width of the index buffers specifically.
	resourceDesc.Width = indexBufferSize;

	// Allocate the upload heap for the index buffer on the GPU.
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
											IID_PPV_ARGS(&indexUpload));
	if (FAILED(result))
	{
		return false;
	}

	// Set the resource name for debugging.
	indexUpload->SetName(L"Index Buffer Upload Heap");

	// Set the width of the matrix buffers specifically.
	resourceDesc.Width = matrixBufferSize;

	// Allocate the upload heap for the matrix buffer on the GPU.
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
											IID_PPV_ARGS(&matrixUpload));
	if (FAILED(result))
	{
		return false;
	}

	// Set the resource name for debugging.
	matrixUpload->SetName(L"Matrix Buffer Upload Heap");

	// Change the heap type to create the upload buffer.
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	//
	resourceDesc.Width = vertexBufferSize;

	// Allocate room for the vertex buffer on the GPU.
	// This is the buffer that the vertex data will get copied to, and that the shaders will use.
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
											D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
											IID_PPV_ARGS(&vertexBuffer));
	if (FAILED(result))
	{
		return false;
	}

	// Set the resource name for debugging.
	vertexBuffer->SetName(L"Vertex Buffer Heap");

	// Set the width of the index buffers specifically.
	resourceDesc.Width = indexBufferSize;

	// Allocate room for the index buffer on the GPU.
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
											D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
											IID_PPV_ARGS(&indexBuffer));
	if (FAILED(result))
	{
		return false;
	}

	// Set the resource name for debugging.
	indexBuffer->SetName(L"Index Buffer Heap");

	// Set the width of the matrix buffers specifically.
	resourceDesc.Width = matrixBufferSize;

	// Allocate room for the index buffer on the GPU.
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
											D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
											IID_PPV_ARGS(&matrixBuffer));
	if (FAILED(result))
	{
		return false;
	}

	// Set the resource name for debugging.
	matrixBuffer->SetName(L"Matrix Buffer Heap");

	return true;
}


bool PipelineClass::SetMatrixBuffer(XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix)
{
	HRESULT result;
	MatrixBufferType* dataPtr;
	D3D12_RESOURCE_BARRIER barrierDesc;


	// Transpose the matrices to prepare them for the shader.
	worldMatrix = XMMatrixTranspose(worldMatrix);
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projectionMatrix = XMMatrixTranspose(projectionMatrix);

	// Lock the constant buffer so it can be written to.
	result = matrixUpload->Map(0, nullptr, (void**)&dataPtr);
	if (FAILED(result))
	{
		return false;
	}
	
	// Copy the matrices into the constant buffer.
	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	// Unlock the constant buffer.
	matrixUpload->Unmap(0, nullptr);
	
	//
	m_commandList->CopyBufferRegion(matrixBuffer, 0, matrixUpload, 0, sizeof(MatrixBufferType));
	
	//
	ZeroMemory(&barrierDesc, sizeof(barrierDesc));

	//
	barrierDesc.Type =						D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Flags =						D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.Transition.pResource =		matrixBuffer;
	barrierDesc.Transition.StateBefore =	D3D12_RESOURCE_STATE_COPY_DEST;
	barrierDesc.Transition.StateAfter =		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrierDesc.Transition.Subresource =	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	//
	m_commandList->ResourceBarrier(1, &barrierDesc);

	return true;
}


bool PipelineClass::RenderPipeline(unsigned int frameIndex, ID3D12Resource* backBuffer,
									D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE depthHandle)
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	float color[4];


	// Reset (re-use) the memory associated command allocator.
	result = m_commandAllocator[frameIndex]->Reset();
	if (FAILED(result))
	{
		return false;
	}

	// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
	result = m_commandList->Reset(m_commandAllocator[frameIndex], m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}

	// Record commands in the command list now.
	// Start by setting the resource barrier.
	barrier.Flags =						D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource =		backBuffer;
	barrier.Transition.StateBefore =	D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter =		D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource =	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type =						D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	// Set the back buffer as the render target and specify the depth buffer.
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthHandle);

	// Then set the color to clear the window to.
	color[0] = 0.0f;
	color[1] = 0.0f;
	color[2] = 0.0f;
	color[3] = 1.0f;
	m_commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);

	m_commandList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Set the vertex input layout.
	//m_commandList->IASetInputLayout(m_layout);

	// Render the geometry.
	//deviceContext->DrawIndexed(indexCount, 0, 0);

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

	return true;
}
