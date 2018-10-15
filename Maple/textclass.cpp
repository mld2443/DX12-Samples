////////////////////////////////////////////////////////////////////////////////
// Filename: textclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "textclass.h"



TextClass::TextClass()
{
	m_brush = nullptr;
	m_format = nullptr;
}


TextClass::TextClass(const TextClass&)
{
}


TextClass::~TextClass()
{
}


bool TextClass::Initialize(IDWriteFactory* dwriteFactory,
	ID2D1DeviceContext* deviceContext,
	float fontSize,
	WCHAR* fontName)
{
	HRESULT result;


	// Initialize drawing window.
	SetDrawWindow(0, 0, 100, 100);

	// Initialize empty text string
	SetTextString(L"");

	// Create our solid white brush.
	result = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_brush);
	if (FAILED(result))
	{
		return false;
	}

	// Create our text format with specified parameters.
	result = dwriteFactory->CreateTextFormat(fontName, nullptr, DWRITE_FONT_WEIGHT_LIGHT,
		DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontSize, L"en-US", &m_format);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}


void TextClass::Shutdown()
{
	// Release the format object.
	if (m_format)
	{
		m_format->Release();
		m_format = nullptr;
	}

	// Release the brush object.
	if (m_brush)
	{
		m_brush->Release();
		m_brush = nullptr;
	}

	return;
}


bool TextClass::Render(ID2D1DeviceContext* deviceContext)
{
	HRESULT result;


	deviceContext->BeginDraw();

	deviceContext->DrawTextW(m_string.c_str(), (UINT32)m_string.size(), m_format, m_drawWindow, m_brush);

	result = deviceContext->EndDraw();
	if (FAILED(result))
	{
		return false;
	}

	return true;
}


void TextClass::SetDrawWindow(float xLeft, float yTop, float xRight, float yBottom)
{
	m_drawWindow = D2D1::RectF(xLeft, yTop, xRight, yBottom);
}


void TextClass::SetBrushColor(D2D1::ColorF& color)
{
	m_brush->SetColor(color);
}


void TextClass::SetTextString(const std::wstring& text)
{
	m_string = text;
}
