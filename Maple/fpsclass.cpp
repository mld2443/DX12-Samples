///////////////////////////////////////////////////////////////////////////////
// Filename: fpsclass.cpp
///////////////////////////////////////////////////////////////////////////////
#include "fpsclass.h"


FpsClass::FpsClass()
{
}


FpsClass::FpsClass(const FpsClass& other)
{
}


FpsClass::~FpsClass()
{
}


void FpsClass::Initialize(float window)
{
	m_window = window;

	return;
}


void FpsClass::Frame()
{
	unsigned long currentTime, cutoffTime;


	// Create a window of time which we care about.
	currentTime = timeGetTime();
	cutoffTime = currentTime - ((unsigned long)m_window * 1000);

	// Add the current frame to the list.
	m_frametimes.push_front(currentTime);

	// Remove old frames from the list that are no longer in our window.
	while (m_frametimes.back() < cutoffTime)
	{
		m_frametimes.pop_back();
	}
}


float FpsClass::GetFps()
{
	float fps, framesInWindow;


	// Get number of frames that have been drawn during our rolling window.
	framesInWindow = (float)m_frametimes.size();

	// Calculate the average framerate over that window.
	fps = framesInWindow / m_window;

	return fps;
}
