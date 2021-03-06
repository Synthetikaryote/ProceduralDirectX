#pragma once
#include <directxmath.h>
using namespace DirectX;
class Model;

struct MovementBinds {
	unsigned char forward, left, back, right, up, down;
};

struct RaycastResult {
	bool didHit;
	XMFLOAT4 hitLocation;
};

class Camera {
public:
	Camera(float fieldOfView, float aspect, float nearPlane, float farPlane);
	~Camera();

	XMFLOAT3 position, rotation, velocity;
	float yaw, pitch;
	bool yawPitchPosDirty = true;
	XMFLOAT3 sensitivity;
	MovementBinds binds;

	void SetFocus(Model* focus);
	Model* focus;
	float focusDist;
	float focusLinearZoom;
	float zoomBase;
	float focusYaw;
	float focusPitch;

	XMFLOAT4X4 rot;
	XMFLOAT4X4 view;
	XMFLOAT4X4 proj;

	bool Update(float elapsed);
	RaycastResult ScreenRaycastToModelSphere(Model* model, int x, int y);
	void UpdateState();
};

