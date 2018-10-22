////////////////////////////////////////////////////////////////////////////////
// Filename: solopipelineclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "solopipelineclass.h"


SoloPipelineClass::SoloPipelineClass()
{
}


SoloPipelineClass::SoloPipelineClass(const SoloPipelineClass& other): PipelineInterface(other)
{
}


SoloPipelineClass::~SoloPipelineClass()
{
}


bool SoloPipelineClass::UpdatePipeline(unsigned int frameIndex)
{
	HRESULT result;
	/*D3D12_RESOURCE_BARRIER barrier;
	float color[4];*/
	ID3D12DescriptorHeap* descriptorHeaps[1];


	// Reset the memory that was holding the previously submitted command list.
	result = m_commandAllocator[frameIndex]->Reset();
	if (FAILED(result))
	{
		return false;
	}

	// Reset our command list to prepare it for new commands.
	result = m_commandList->Reset(m_commandAllocator[frameIndex], m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}

	//NOTE: This should probably be done in ResourcesClass.
	/*// Create a barrier to wait for frame to finish presenting.
	barrier.Flags =						D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource =		m_backBufferRenderTarget[frameIndex];
	barrier.Transition.StateBefore =	D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter =		D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource =	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type =						D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	// Specify the render target and depth buffer for the output merger stage.
	m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Clear the render target to a soft gray.
	color[0] = 0.2f;
	color[1] = 0.2f;
	color[2] = 0.2f;
	color[3] = 1.0f;
	m_commandList->ClearRenderTargetView(rtv, color, 0, nullptr);

	// Clear the depth buffer.
	m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);*/

	// Declare the root signature.
	m_commandList->SetGraphicsRootSignature(m_rootSignature);

	//TODO: fix these next lines.
	// set constant buffer descriptor heap
	//descriptorHeaps[0] = mainDescriptorHeap[frameIndex];
	//m_commandList->SetDescriptorHeaps(1, descriptorHeaps);

	// set the root descriptor table 0 to the constant buffer descriptor heap
	//m_commandList->SetGraphicsRootDescriptorTable(0, mainDescriptorHeap[frameIndex]->GetGPUDescriptorHandleForHeapStart());

	// Set our window viewport.
	m_commandList->RSSetViewports(1, &m_viewport);
	//m_commandList->RSSetScissorRects(1, &scissorRect);

	// Specify the topology of our geometry.
	//m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Specify the vertices.
	//m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// Specify the indicies.
	//m_commandList->IASetIndexBuffer(&indexBufferView);

	// Issue the draw command.
	//m_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	/*// Re-order the barrier states so it can put the RTV into presenting.
	barrier.Transition.StateAfter =		D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateBefore =	D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_commandList->ResourceBarrier(1, &barrier);*/

	return true;
}


ID3D12CommandList* SoloPipelineClass::ClosePipeline()
{
	HRESULT result;


	// Close the command list so it can be submitted to a command queue.
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return nullptr;
	}

	return m_commandList;
}


bool SoloPipelineClass::InitializePipeline(ID3D12Device* device, int screenWidth, int screenHeight)
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

	// Setup the viewport for rendering.
	m_viewport.Width =		(float)screenWidth;
	m_viewport.Height =		(float)screenHeight;
	m_viewport.MinDepth =	0.0f;
	m_viewport.MaxDepth =	1.0f;
	m_viewport.TopLeftX =	0.0f;
	m_viewport.TopLeftY =	0.0f;

	//TODO: Set up vertex (and index?) buffers for the shader.


	//TODO: Set up view matrix communication with the shader here too.
	//TODO: Set up stencil and raster views and stuff here.

	return true;
}


void SoloPipelineClass::ShutdownPipeline()
{
	// This particular pipeline does not currently have any
	// implementation specific resources that need to be shutdown.
	return;
}
