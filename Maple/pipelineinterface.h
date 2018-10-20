////////////////////////////////////////////////////////////////////////////////
// Filename: pipelineinterface.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


//////////////
// INCLUDES //
//////////////
#include <d3d12.h>


/////////////////
// DEFINITIONS //
/////////////////
#define FRAME_BUFFER_COUNT 2


////////////////////////////////////////////////////////////////////////////////
// Interface name: PipelineInterface
////////////////////////////////////////////////////////////////////////////////
class PipelineInterface
{
public:
	PipelineInterface();
	PipelineInterface(const PipelineInterface&);
	~PipelineInterface();

	bool Initialize(ID3D12Device*, HWND, unsigned int);
	void Shutdown();

protected:
	virtual bool InitializePipeline(ID3D12Device*) = 0;
	virtual void ShutdownPipeline() = 0;

protected:
	ID3D12RootSignature*		m_rootSignature;
	ID3D12CommandAllocator*		m_commandAllocator[FRAME_BUFFER_COUNT];
	ID3D12GraphicsCommandList*	m_commandList;
	ID3D12PipelineState*		m_pipelineState;
};

