#pragma once
#include "pch.h"

struct InputCommands;

class Camera
{
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Matrix = DirectX::SimpleMath::Matrix;

public:

	void Update(const InputCommands& inputCommands);

	Vector3 GetPosition() const;
	Matrix GetViewMatrix() const;

private:
	Vector3 m_scale;
	Vector3 m_rotation;
	Vector3 m_position{0.f, 3.7f, -3.5f};

	Vector3 m_forward;
	Vector3 m_lookAt;

	float m_speed = 0.3f;
	float m_rotSpeed = 3.f;
};

