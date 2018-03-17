#pragma once
#include "pch.h"

struct InputCommands;

class Camera
{
public:

	void Update(const InputCommands& inputCommands);

	DirectX::SimpleMath::Vector3 GetPosition() const;
	DirectX::SimpleMath::Matrix GetViewMatrix() const;

private:
	DirectX::SimpleMath::Vector3 m_scale{};
	DirectX::SimpleMath::Vector3 m_rotation{};
	DirectX::SimpleMath::Vector3 m_position{0.f, 3.7f, -3.5f};

	DirectX::SimpleMath::Vector3 m_forward{};
	DirectX::SimpleMath::Vector3 m_lookAt{};

	float m_speed = 0.3f;
	float m_rotSpeed = 3.f;
};

