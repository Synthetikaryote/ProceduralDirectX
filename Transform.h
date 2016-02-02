#pragma once
class Transform
{
public:
	Transform();
	~Transform();

	float x = 0.0f, y = 0.0f, z = 0.0f;
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
};

