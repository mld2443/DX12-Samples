////////////////////////////////////////////////////////////////////////////////
// Filename: pipelineinterface.cpp
////////////////////////////////////////////////////////////////////////////////
#include "pipelineinterface.h"


PipelineInterface::PipelineInterface()
{
	m_rootSignature = nullptr;
	m_pipelineState = nullptr;
	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_commandAllocator[i] = nullptr;
	}
	m_commandList = nullptr;
}


PipelineInterface::PipelineInterface(const PipelineInterface& other)
{
}


PipelineInterface::~PipelineInterface()
{
}


bool PipelineInterface::Initialize(ID3D12Device* device, unsigned int frameIndex)
{
	bool result;


	// Initialize pipeline specific resources.
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


void PipelineInterface::Shutdown()
{
	// Release the command list.
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}

	// Release the command allocators.
	for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		if (m_commandAllocator[i])
		{
			m_commandAllocator[i]->Release();
			m_commandAllocator[i] = nullptr;
		}
	}

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


bool PipelineInterface::InitializeCommandList(ID3D12Device* device, unsigned int frameIndex)
{
	HRESULT result;


	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		// Create command allocators, one for each frame.
		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]));
		if (FAILED(result))
		{
			return false;
		}
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

	return true;
}

