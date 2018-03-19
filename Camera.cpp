#include "Camera.h"
#include "InputCommands.h"

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
	using namespace DirectX::SimpleMath;

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
	m_rotation.x = clamp(m_rotation.x, -90.f, 90.f);

	//create look direction from Euler angles in m_camOrientation	
	constexpr float DEG_TO_RAD = (3.1415f / 180.f);
	float cosY = cosf(m_rotation.y * DEG_TO_RAD);
	float cosP = cosf(m_rotation.x * DEG_TO_RAD);

	float sinY = sinf(m_rotation.y * DEG_TO_RAD);
	float sinP = sinf(m_rotation.x * DEG_TO_RAD);

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
	using namespace DirectX::SimpleMath;
	return Matrix::CreateLookAt(m_position, m_lookAt, Vector3::UnitY);
}
