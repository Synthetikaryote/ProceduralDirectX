#include "Camera.h"
#include "Uber.h"
#include "Model.h"
#include "Transform.h"

Camera::Camera(float fieldOfView, float aspect, float nearPlane, float farPlane) {
	position = { 0.f, 0.f, -3.f };
	rotation = { 0.f, 0.f, 0.f };
	velocity = { 0.f, 0.f, 0.f };
	yaw = 0.f;
	pitch = 0.f;
	sensitivity = { 0.0002f, 0.0002f, 0.0002f };

	focusLinearZoom = 0.0f;
	zoomBase = 5.0f;
	focusDist = 100.0f;
	focus = nullptr;

	// create the projection matrix
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, aspect, nearPlane, farPlane);
	XMStoreFloat4x4(&proj, projectionMatrix);

	// create an orthographic projection matrix for 2D UI rendering.
	//XMMATRIX orthoMatrix = XMMatrixOrthographicLH(static_cast<float>(Uber::I().windowWidth), static_cast<float>(Uber::I().windowHeight), screenNear, screenDepth);
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
		focusYaw = TWOPI - (yaw + PI);
		//focusYaw = atan2f(dz, dx) + HALFPI;
		focusPitch = asinf(-dy / focusDist);
		position = XMFLOAT3(0.0f, 0.0f, -focusDist);
		yaw = 0.0f;
		pitch = 0.0f;
	}
	else {
		//yaw = TWOPI - (focusYaw + PI);
	}
	this->focus = focus;

	UpdateState();
}

bool Camera::Update(float elapsed) {
	bool cameraChanged = false;

	if (Uber::I().windowIsFocused) {
		// zoom
		focusLinearZoom = max(-10.0f, min(10.0f, focusLinearZoom + sensitivity.z * -Uber::I().mouseState.lZ));
		float boundingRadius = 1.0f; // to do
		float closeness = powf(zoomBase, focusLinearZoom);
		focusDist = boundingRadius + closeness;
		float zoomFactor = min(1.0f, closeness * 0.2f);

		// hold right click to rotate
		if (Uber::I().mouseState.rgbButtons[1]) {
			if (Uber::I().cursorVisible) {
				ShowCursor(false);
				Uber::I().cursorVisible = false;
				GetCursorPos(&Uber::I().savedMousePos);
			}

			if (GetActiveWindow() == Uber::I().hWnd) {
				RECT r;
				GetWindowRect(Uber::I().hWnd, &r);
				SetCursorPos(Uber::I().savedMousePos.x, Uber::I().savedMousePos.y);
			}

			if (focus) {
				float newFocusYaw = fmodf(focusYaw + sensitivity.x * zoomFactor * Uber::I().mouseState.lX, TWOPI);
				float newFocusPitch = max(-HALFPI, min(fmodf(focusPitch + sensitivity.y * zoomFactor * -Uber::I().mouseState.lY, TWOPI), HALFPI));
				if (newFocusYaw != focusYaw || newFocusPitch != focusPitch) {
					cameraChanged = true;
					focusYaw = newFocusYaw;
					focusPitch = newFocusPitch;
				}
			}
			else {
				yaw = fmodf(yaw + sensitivity.x * Uber::I().mouseState.lX, TWOPI);
				pitch = max(-HALFPI, min(fmodf(pitch + sensitivity.y * Uber::I().mouseState.lY, TWOPI), HALFPI));
				yawPitchPosDirty = true;
			}
		}
		else {
			if (!Uber::I().cursorVisible) {
				ShowCursor(true);
				Uber::I().cursorVisible = true;
				SetCursorPos(Uber::I().savedMousePos.x, Uber::I().savedMousePos.y);
			}
		}

		// update position
		if (focus) {
			//yaw = TWOPI - (focusYaw - PI);
			//pitch = focusPitch;
			//position.x = focusDist * cos(focusYaw + HALFPI) * cos(focusPitch);
			//position.y = focusDist * sin(focusPitch);
			//position.z = focusDist * sin(focusYaw + HALFPI) * cos(focusPitch);
			if (position.z != -focusDist) {
				position.z = -focusDist;
				yawPitchPosDirty = true;
			}
		}
		else {
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
				yawPitchPosDirty = true;
			}
		}
		// if the camera's position or rotation changed, regenerate the view matrix
		cameraChanged |= yawPitchPosDirty;
		if (yawPitchPosDirty)
			UpdateState();
	}
	return cameraChanged;
}

void Camera::UpdateState() {
	XMVECTOR upVector = XMLoadFloat3(&worldUp);
	XMVECTOR forwardVector = XMLoadFloat3(&worldForward);
	XMVECTOR positionVector = XMLoadFloat3(&position);
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.f);
	// rotate the forward and up according to the camera rotation
	forwardVector = XMVector3TransformCoord(forwardVector, rotationMatrix);
	upVector = XMVector3TransformCoord(upVector, rotationMatrix);
	// move the forward vector to the target position
	forwardVector = XMVectorAdd(positionVector, forwardVector);
	// create the view matrix
	XMMATRIX viewMatrix = XMMatrixLookAtLH(positionVector, forwardVector, upVector);
	XMStoreFloat4x4(&view, viewMatrix);
	yawPitchPosDirty = false;
}

RaycastResult Camera::ScreenRaycastToModelSphere(Model* model, int x, int y) {
	// raycast the cursor position to find the spot on the model the camera is focusing on
	// reference: http://richardssoftware.net/Home/Post/23
	// put the mouse coords into -1, 1 space as the origin
	float vx = (2.0f * x / Uber::I().windowWidth - 1.0f) / proj._11;
	float vy = (-2.0f * y / Uber::I().windowHeight + 1.0f) / proj._22;
	Transform& t = *model->transform;
	XMVECTOR start = XMLoadFloat4(&XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
	XMVECTOR end = XMLoadFloat4(&XMFLOAT4(vx, vy, 1.0f, 1.0f));
	XMVECTOR dir = XMVectorSubtract(end, start);
	XMMATRIX viewMatrix = XMLoadFloat4x4(&view);
	XMMATRIX inverseView = XMMatrixInverse(nullptr, viewMatrix);
	start = XMVector4Transform(start, inverseView);
	end = XMVector4Transform(end, inverseView);
	dir = XMVector4Transform(dir, inverseView);
	dir = XMVector3Normalize(dir);
	// find the intersection with the sphere
	// reference: https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
	// d = -(l . (o - c)) +- sqrt( (l . (o - c))^2 - (||o - c||^2 - r^2) )
	// l = dir, o = start, c = center, r = 1
	// if inside the sqrt is less than 0, there is no intersection
	XMVECTOR center = XMLoadFloat4(&XMFLOAT4(t.x, t.y, t.z, 1.0f));
	// delta = (o - c)
	XMVECTOR delta = XMVectorSubtract(start, center);
	XMFLOAT3 dotResult, lenResult;
	XMStoreFloat3(&dotResult, XMVector3Dot(dir, delta));
	XMStoreFloat3(&lenResult, XMVector3LengthSq(delta));
	float underSqrt = dotResult.x * dotResult.x - lenResult.x + 1.0f;
	XMFLOAT4 cursorPosition = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	RaycastResult result;
	if (underSqrt >= 0) {
		// 2 intersections, but always take the closer one
		float sqrtResult = sqrtf(underSqrt);
		float dist = -dotResult.x - sqrtResult;
		XMStoreFloat4(&result.hitLocation, XMVectorAdd(start, XMVectorScale(dir, dist)));
		result.didHit = true;
	}
	else {
		result.didHit = false;
	}
	return result;
}