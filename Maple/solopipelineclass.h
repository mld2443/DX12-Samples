////////////////////////////////////////////////////////////////////////////////
// Filename: solopipelineclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


//////////////
// INCLUDES //
//////////////
#include <DirectXMath.h>


///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "pipelineinterface.h"
#include "color.vs.h"
#include "color.ps.h"

using namespace DirectX;

////////////////////////////////////////////////////////////////////////////////
// Class name: SoloPipelineClass
////////////////////////////////////////////////////////////////////////////////
class SoloPipelineClass : public PipelineInterface
{
public:
	SoloPipelineClass();
	SoloPipelineClass(const SoloPipelineClass&);
	~SoloPipelineClass();

	bool UpdatePipeline(unsigned int) override;
	ID3D12CommandList* ClosePipeline() override;

protected:
	bool InitializePipeline(ID3D12Device*, int, int) override;
	void ShutdownPipeline() override;

private:
	D3D12_VIEWPORT	m_viewport;

	XMMATRIX	m_projectionMatrix;
	XMMATRIX	m_worldMatrix;
	XMMATRIX	m_orthoMatrix;
};
