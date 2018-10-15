////////////////////////////////////////////////////////////////////////////////
// Filename: resourcesclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


/////////////
// LINKING //
/////////////
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")


//////////////
// INCLUDES //
//////////////
#include <d3d11on12.h>
#include <d3d12.h>
#include <d2d1_3.h>
#include <d2d1_1helper.h>
#include <dwrite_3.h>
#include <dxgi1_4.h>


/////////////////
// DEFINITIONS //
/////////////////
#define FRAME_BUFFER_COUNT 2


////////////////////////////////////////////////////////////////////////////////
// Class name: ResourcesClass
////////////////////////////////////////////////////////////////////////////////
class ResourcesClass
{
public:
	ResourcesClass();
	ResourcesClass(const ResourcesClass&);
	~ResourcesClass();

	bool Initialize(int, int, HWND, bool, bool);
	void Shutdown();

	bool BeginScene(float, float, float, float);
	void BeginDirect2D();
	void EndDirect2D();
	bool EndScene();

	ID2D1Device4* GetDirect2DDevice();
	ID2D1DeviceContext4* GetDirect2DDeviceContext();
	IDWriteFactory* GetDirectWriteFactory();

private:
	bool InitializeDirect3D12(int, int, HWND, bool, bool);
	bool InitializeDirect2D();
	bool InitializeDirectWrite();

	void ShutdownDirect3D();
	void ShutdownDirect2D();
	void ShutdownDirectWrite();

private:
	bool				m_vsync_enabled;
	int					m_videoCardMemory;
	char				m_videoCardDescription[128];
	IDXGISwapChain3*	m_swapChain;

	// D3D12
	ID3D12Device*				m_d3d12Device;
	ID3D12CommandQueue*			m_commandQueue;
	ID3D12DescriptorHeap*		m_renderTargetViewHeap;
	ID3D12Resource*				m_backBufferRenderTarget[FRAME_BUFFER_COUNT];
	unsigned int				m_bufferIndex;
	ID3D12CommandAllocator*		m_commandAllocator;
	ID3D12GraphicsCommandList*	m_commandList;
	ID3D12PipelineState*		m_pipelineState;
	ID3D12Fence*				m_fence;
	HANDLE						m_fenceEvent;
	unsigned long long			m_fenceValue;

	// D3D11 & D2D
	ID3D11On12Device*		m_d3d11On12Device;
	ID3D11DeviceContext*	m_d3d11DeviceContext;
	ID2D1Device4*			m_d2dDevice;
	ID2D1DeviceContext4*	m_d2dDeviceContext;
	ID3D11Resource*			m_wrappedBackBuffers[FRAME_BUFFER_COUNT];
	ID2D1Bitmap1*			m_bitmaps[FRAME_BUFFER_COUNT];

	// DirectWrite
	IDWriteFactory*	m_dWriteFactory;
};
