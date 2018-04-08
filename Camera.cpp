#include "Camera.h"
#include "InputCommands.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
	template<typename T>
	inline T clamp(const T& val, const T& min, const T& max)
	{
		return (val > max ? max : (val < min ? min : val));
	}
}

void Camera::Update(const InputCommands & inputCommands)
{
	if (inputCommands.rotRight)
	{
		m_rotation.y -= m_rotSpeed;
	}
	if (inputCommands.rotLeft)
	{
		m_rotation.y += m_rotSpeed;
	}

	m_rotation.y += inputCommands.mouseDX;
	m_rotation.x += inputCommands.mouseDY;
	m_rotation.x = clamp(m_rotation.x, -89.f, 89.f);

	//create look direction from Euler angles in m_camOrientation	
	float cosY = cosf(XMConvertToRadians(m_rotation.y));
	float cosP = cosf(XMConvertToRadians(m_rotation.x));

	float sinY = sinf(XMConvertToRadians(m_rotation.y));
	float sinP = sinf(XMConvertToRadians(m_rotation.x));

	m_forward.x = sinY * cosP;
	m_forward.y = sinP;
	m_forward.z = cosP * cosY;
	m_forward.Normalize();

	//create right vector from look Direction
	Vector3 right;
	m_forward.Cross(Vector3::UnitY, right);
    right.Normalize();

	//process input and update stuff
	if (inputCommands.forward)
	{
		m_position += m_forward*m_speed;
	}
	if (inputCommands.back)
	{
		m_position -= m_forward*m_speed;
	}
	if (inputCommands.right)
	{
		m_position += right*m_speed;
	}
	if (inputCommands.left)
	{
		m_position -= right*m_speed;
	}

	//update lookat point
	m_lookAt = m_position + m_forward;
}

DirectX::SimpleMath::Vector3 Camera::GetPosition() const
{
	return m_position;
}

DirectX::SimpleMath::Matrix Camera::GetViewMatrix() const
{
	return Matrix::CreateLookAt(m_position, m_lookAt, Vector3::UnitY);
}
