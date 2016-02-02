#include "Camera.h"
#include "Uber.h"
#include "Model.h"
#include "Transform.h"

Camera::Camera() {
	position = { 0.f, 0.f, -3.f };
	rotation = { 0.f, 0.f, 0.f };
	up = { 0.f, 1.f, 0.f };
	forward = { 0.f, 0.f, 1.f };
	velocity = { 0.f, 0.f, 0.f };
	yaw = 0.f;
	pitch = 0.f;
	sensitivity = { 0.0002f, 0.0002f, 0.0002f };

	focusLinearZoom = 0.0f;
	zoomBase = 5.0f;
	focusDist = 100.0f;
	focus = nullptr;
}


Camera::~Camera() {
}

void Camera::SetFocus(Model* focus) {
	if (focus && this->focus != focus) {
		float dx = focus->transform->x - position.x;
		float dy = focus->transform->y - position.y;
		float dz = focus->transform->z - position.z;
		focusDist = sqrtf(dx * dx + dy * dy + dz * dz);
		focusLinearZoom = logf(focusDist) / logf(zoomBase);
		focusYaw = atan2f(dz, dx) + HALFPI;
		focusPitch = asinf(-dy / focusDist);
	}
	else {
		yaw = TWOPI - (focusYaw + PI);
	}
	this->focus = focus;
}

void Camera::Update(float elapsed) {
	if (GetActiveWindow() == Uber::I().hWnd) {
		RECT r;
		GetWindowRect(Uber::I().hWnd, &r);
		SetCursorPos(static_cast<int>(static_cast<float>(r.right - r.left) * 0.5f), static_cast<int>(static_cast<float>(r.bottom - r.top) * 0.5f));
	}

	if (focus) {
		float boundingRadius = 1.0f; // to do
		focusLinearZoom = max(-10.0f, min(10.0f, focusLinearZoom + sensitivity.z * -Uber::I().mouseState.lZ));
		float closeness = powf(zoomBase, focusLinearZoom);
		focusDist = boundingRadius + closeness;
		float zoomFactor = min(1.0f, closeness * 0.2f);
		focusYaw = fmodf(focusYaw + sensitivity.x * zoomFactor * Uber::I().mouseState.lX, TWOPI);
		focusPitch = max(-HALFPI, min(fmodf(focusPitch + sensitivity.y * zoomFactor * -Uber::I().mouseState.lY, TWOPI), HALFPI));
		yaw = TWOPI - (focusYaw - PI);
		pitch = focusPitch;
		position.x = focusDist * cos(focusYaw + HALFPI) * cos(focusPitch);
		position.y = focusDist * sin(focusPitch);
		position.z = focusDist * sin(focusYaw + HALFPI) * cos(focusPitch);
	}
	else {
		yaw = fmodf(yaw + sensitivity.x * Uber::I().mouseState.lX, TWOPI);
		pitch = max(-HALFPI, min(fmodf(pitch + sensitivity.y * Uber::I().mouseState.lY, TWOPI), HALFPI));

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
