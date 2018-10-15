////////////////////////////////////////////////////////////////////////////////
// Filename: resourcesclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


/////////////
// LINKING //
/////////////
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")


//////////////
// INCLUDES //
//////////////
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
	bool Render(ID3D12CommandList**, UINT);

	unsigned int GetBufferIndex();

private:
	bool InitializeDirect3D(int, int, HWND, bool, bool);
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

	// Direct3D
	ID3D12Device*				m_direct3DDevice;
	ID3D12CommandQueue*			m_commandQueue;
	ID3D12DescriptorHeap*		m_renderTargetViewHeap;
	ID3D12Resource*				m_backBufferRenderTarget[2];
	unsigned int				m_bufferIndex;

	// Direct2D
	ID2D1Device4*			m_direct2DDevice;
	ID2D1DeviceContext4*	m_direct2DDeviceContext;
	ID2D1Bitmap1*			m_bitmap;

	// DirectWrite
	IDWriteFactory*	m_directWriteFactory;
};
