#include "Camera.h"
#include "Uber.h"

Camera::Camera() {
	position = {0.f, 0.f, -3.f};
	rotation = {0.f, 0.f, 0.f};
	up = {0.f, 1.f, 0.f};
	forward = {0.f, 0.f, 1.f};
	velocity = {0.f, 0.f, 0.f};
	yaw = 0.f;
	pitch = 0.f;
	sensitivity = 0.0002f;
}


Camera::~Camera() {
}

void Camera::Update(float elapsed) {
	if (GetActiveWindow() == Uber::I().hWnd) {
		RECT r;
		GetWindowRect(Uber::I().hWnd, &r);
		SetCursorPos((float)(r.right - r.left) * 0.5f, (float)(r.bottom - r.top) * 0.5f);
	}

	if (focus) {

	}
	else {
		yaw = fmodf(yaw + sensitivity * Uber::I().mouseState.lX, TWOPI);
		pitch = max(-HALFPI, min(fmodf(pitch + sensitivity * Uber::I().mouseState.lY, TWOPI), HALFPI));

		float speed = 2.f;
		float x = (IsKeyDown(binds.left) ? -1.f : 0.f) + (IsKeyDown(binds.right) ? 1.f : 0.f);
		float y = (IsKeyDown(binds.down) ? -1.f : 0.f) + (IsKeyDown(binds.up) ? 1.f : 0.f);
		float z = (IsKeyDown(binds.back) ? -1.f : 0.f) + (IsKeyDown(binds.forward) ? 1.f : 0.f);
		float lenSq = x * x + y * y + z * z;
		if (lenSq > 0.f) {
			float lenInv = 1.f / sqrt(lenSq);
			x *= lenInv; y *= lenInv; z *= lenInv;
			float py = y * cos(pitch) - z * sin(pitch), pz = y * sin(pitch) + z * cos(pitch);
			float yx = x * cos(yaw) + pz * sin(yaw), yz = -x * sin(yaw) + pz * cos(yaw);
			x = yx; y = py; z = yz;
			float mult = speed * elapsed;
			x *= mult; y *= mult; z *= mult;
			position.x += x; position.y += y; position.z += z;
		}
	}
}
