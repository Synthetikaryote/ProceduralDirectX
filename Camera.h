#pragma once
#include <directxmath.h>
class Mesh;

using namespace DirectX;
class Camera {
public:
	Camera();
	~Camera();
	void Update(float elapsed);

	XMFLOAT3 position, rotation, up, forward, velocity;
	float yaw, pitch;
	float sensitivity;

	Mesh* focus;
};

