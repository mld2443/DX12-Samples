////////////////////////////////////////////////////////////////////////////////
// Filename: cameraclass.h
////////////////////////////////////////////////////////////////////////////////
#pragma once


//////////////
// INCLUDES //
//////////////
#include <directxmath.h>
using namespace DirectX;


///////////////
// CONSTANTS //
///////////////
#define PI_180 0.0174532925f


////////////////////////////////////////////////////////////////////////////////
// Class name: CameraClass
////////////////////////////////////////////////////////////////////////////////
class CameraClass
{
public:
	CameraClass();
	CameraClass(const CameraClass&);
	~CameraClass();

	void SetPosition(float, float, float);
	void SetLookDirection(float, float, float);
	void SetRotation(float, float, float);

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLookDirection();
	XMFLOAT3 GetRotation();

	void Render();
	void GetViewMatrix(XMMATRIX&);

private:
	XMFLOAT3	m_position;
	XMFLOAT3	m_lookDirection;
	XMFLOAT3	m_rotation;
	XMMATRIX	m_viewMatrix;
};
