////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "graphicsclass.h"


GraphicsClass::GraphicsClass()
{
    m_Camera = nullptr;
	m_Resources = nullptr;
	m_Text = nullptr;
}


GraphicsClass::GraphicsClass(const GraphicsClass& other)
{
}


GraphicsClass::~GraphicsClass()
{
}


bool GraphicsClass::Initialize(int screenHeight, int screenWidth, HWND hwnd)
{
	bool result;


	// Create the camera object.
	m_Camera = new CameraClass;
	if (!m_Camera)
	{
		return false;
	}

	// Set the initial parameters of the camera.
	m_Camera->SetPosition(0.0f, 0.0f, -5.0f);
	m_Camera->SetLookDirection(0.0f, 0.0f, 1.0f);

	// Create the resources object.
	m_Resources = new ResourcesClass;
	if (!m_Resources)
	{
		return false;
	}

	// Initialize the resources object.
	result = m_Resources->Initialize(screenHeight, screenWidth, hwnd, VSYNC_ENABLED, FULL_SCREEN);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
		return false;
	}

	// Create the text object.
	m_Text = new TextClass;
	if (!m_Text)
	{
		return false;
	}

	// Initialize the text object.
	result = m_Text->Initialize(m_Resources->GetDirectWriteFactory(), m_Resources->GetDirect2DDeviceContext(), 20.0f, L"Consolas");
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the text object.", L"Error", MB_OK);
		return false;
	}

	// Set the window of the text object.
	m_Text->SetDrawWindow(3.0f, 0.0f, 150.0f, 150.0f);

	// Set the color of our text object.
	m_Text->SetBrushColor(D2D1::ColorF(D2D1::ColorF::Plum));

	return true;
}


void GraphicsClass::Shutdown()
{
    // Release the Text object.
	if (m_Text)
	{
		m_Text->Shutdown();
		delete m_Text;
		m_Text = nullptr;
	}

	// Release the Resources object.
	if (m_Resources)
	{
		m_Resources->Shutdown();
		delete m_Resources;
		m_Resources = nullptr;
	}

	// Release the camera object.
	if (m_Camera)
	{
		delete m_Camera;
		m_Camera = nullptr;
	}

	return;
}


bool GraphicsClass::Frame(float fps, int cpu, float frameTime)
{
	bool result;
	CString stats;


	// Build the statistics string.
	stats.Format(L"FPS: %.1f\nCPU: %d%%", fps, cpu);

	// Set the stats text string for our text object.
	m_Text->SetTextString(stats.GetString());

	// Render the graphics scene.
	result = Render();
	if (!result)
	{
		return false;
	}

	return true;
}


bool GraphicsClass::Render()
{
	bool result;


	// Generate the view matrix based on the camera's position.
	m_Camera->Render();

	// Use the Direct3D 12 object to render the scene.
	result = m_Resources->BeginScene(0.2f, 0.2f, 0.2f, 1.0f);
	if (!result)
	{
		return false;
	}

	// Signal our resources to hand off the pipeline to d2d.
	m_Resources->BeginDirect2D();

	// Render text on the screen.
	m_Text->Render(m_Resources->GetDirect2DDeviceContext());

	// Signal to the D2D rendering.
	m_Resources->EndDirect2D();

	// Attempt to present the current screen.
	result = m_Resources->EndScene();
	if (!result)
	{
		return false;
	}

	return true;
}
