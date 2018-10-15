////////////////////////////////////////////////////////////////////////////////
// Filename: graphicsclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "graphicsclass.h"


GraphicsClass::GraphicsClass()
{
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


	// Create the Direct3D object.
	m_Resources = new ResourcesClass;
	if (!m_Resources)
	{
		return false;
	}

	// Initialize the Direct3D object.
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
	// Release the Direct3D object.
	if (m_Resources)
	{
		m_Resources->Shutdown();
		delete m_Resources;
		m_Resources = nullptr;
	}

	return;
}


bool GraphicsClass::Frame()
{
	bool result;


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


	// Use the Direct3D object to render the scene.
	result = m_Resources->Render();
	if (!result)
	{
		return false;
	}

	return true;
}