////////////////////////////////////////////////////////////////////////////////
// Filename: systemclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


///////////////////////////////
// PRE-PROCESSING DIRECTIVES //
///////////////////////////////
#define WIN32_LEAN_AND_MEAN


//////////////
// INCLUDES //
//////////////
#include <windows.h>


///////////////////////
// MY CLASS INCLUDES //
///////////////////////
#include "cpuclass.h"
#include "fpsclass.h"
#include "graphicsclass.h"
#include "inputclass.h"
#include "timerclass.h"


////////////////////////////////////////////////////////////////////////////////
// Class name: SystemClass
////////////////////////////////////////////////////////////////////////////////
class SystemClass
{
public:
	SystemClass();
	SystemClass(const SystemClass&);
	~SystemClass();

	bool Initialize();
	void Shutdown();
	void Run();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool Frame();
	bool InitializeWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCWSTR		m_applicationName;
	HINSTANCE	m_hinstance;
	HWND		m_hwnd;

	InputClass*		m_Input;
	GraphicsClass*	m_Graphics;
	FpsClass*		m_Fps;
	CpuClass*		m_Cpu;
	TimerClass*		m_Timer;
};


/////////////////////////
// FUNCTION PROTOTYPES //
/////////////////////////
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


/////////////
// GLOBALS //
/////////////
static SystemClass* g_ApplicationHandle = 0;
