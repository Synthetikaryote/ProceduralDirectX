#pragma once
#include <directxmath.h>
class Model;

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
	XMFLOAT3 sensitivity;
	MovementBinds binds;

	void SetFocus(Model* focus);
	Model* focus;
	float focusDist;
	float focusLinearZoom;
	float zoomBase;
	float focusYaw;
	float focusPitch;
};

