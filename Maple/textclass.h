////////////////////////////////////////////////////////////////////////////////
// Filename: textclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


//////////////
// INCLUDES //
//////////////
#include <d2d1_3.h>
#include <dwrite.h>
#include <string>


////////////////////////////////////////////////////////////////////////////////
// Class name: TextClass
////////////////////////////////////////////////////////////////////////////////
class TextClass
{
public:
	TextClass();
	TextClass(const TextClass&);
	~TextClass();

	bool Initialize(IDWriteFactory*, ID2D1DeviceContext*, float = 36.0f, WCHAR* = L"Segoe UI");
	void Shutdown();

	void SetDrawWindow(float, float, float, float);
	void SetBrushColor(D2D1::ColorF&);
	void SetTextString(const std::wstring&);

	bool Render(ID2D1DeviceContext*);

private:
	ID2D1SolidColorBrush*	m_brush;
	IDWriteTextFormat*		m_format;
	std::wstring			m_string;
	D2D1_RECT_F				m_drawWindow;
};
