#pragma once
#include <directxmath.h>
class Mesh;

struct MovementBinds {
	unsigned char forward, left, back, right, up, down;
};

using namespace DirectX;
class Camera {
public:
	Camera();
	~Camera();
	void Update(float elapsed);

	XMFLOAT3 position, rotation, up, forward, velocity;
	float yaw, pitch;
	float sensitivity;
	MovementBinds binds;

	Mesh* focus;
};

