////////////////////////////////////////////////////////////////////////////////
// Filename: pipelineclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


//////////////
// INCLUDES //
//////////////
#include <d3d12.h>
#include <directxmath.h>


///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "color.ps.h"
#include "color.vs.h"


/////////////
// GLOBALS //
/////////////
#define FRAME_BUFFER_COUNT 2

using namespace DirectX;

////////////////////////////////////////////////////////////////////////////////
// Class name: PipelineClass
////////////////////////////////////////////////////////////////////////////////
class PipelineClass
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

public:
	PipelineClass();
	PipelineClass(const PipelineClass&);
	~PipelineClass();

	bool Initialize(ID3D12Device*, unsigned int);
	void Shutdown();
	bool Render(XMMATRIX, XMMATRIX, XMMATRIX);

private:
	bool InitializePipeline(ID3D12Device*);
	bool InitializeCommandList(ID3D12Device*, unsigned int);
	void ShutdownPipeline();
	void ShutdownCommandList();

	bool AllocateBuffers(ID3D12Device*, UINT64, UINT64, UINT64);
	//bool SetVertexBuffer(VertexType*);
	bool SetMatrixBuffer(XMMATRIX, XMMATRIX, XMMATRIX);

private:
	ID3D12RootSignature*		m_rootSignature;
	ID3D12PipelineState*		m_pipelineState;

	ID3D12CommandAllocator*		m_commandAllocator[FRAME_BUFFER_COUNT];
	ID3D12GraphicsCommandList*	m_commandList;
	ID3D12Fence*				m_fence[FRAME_BUFFER_COUNT];
	HANDLE						m_fenceEvent;
	unsigned long long			m_fenceValue[FRAME_BUFFER_COUNT];

	ID3D12Resource	*vertexBuffer, *indexBuffer, *matrixBuffer;
	ID3D12Resource	*vertexUpload, *indexUpload, *matrixUpload;
};
