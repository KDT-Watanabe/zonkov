#include "DxLib.h"
#include "InputManager.h"
#include <cstring>

InputManager::InputManager()
	: mousePos_(0.0f, 0.0f)
	, prevMousePos_(0.0f, 0.0f)
	, mouseWheelRotation_(0)
{
	memset(currentKeys_, 0, sizeof(currentKeys_));
	memset(prevKeys_, 0, sizeof(prevKeys_));
	memset(currentMouseButtons_, 0, sizeof(currentMouseButtons_));
	memset(prevMouseButtons_, 0, sizeof(prevMouseButtons_));
}

void InputManager::Update()
{
	memcpy(prevKeys_, currentKeys_, sizeof(currentKeys_));
	prevMousePos_ = mousePos_;
	memcpy(prevMouseButtons_, currentMouseButtons_, sizeof(currentMouseButtons_));

	GetHitKeyStateAll(currentKeys_);

	int mx, my;
	GetMousePoint(&mx, &my);
	mousePos_ = Vector2(static_cast<float>(mx), static_cast<float>(my));

	int mouseInput = GetMouseInput();
	currentMouseButtons_[0] = (mouseInput & MOUSE_INPUT_LEFT) != 0;
	currentMouseButtons_[1] = (mouseInput & MOUSE_INPUT_RIGHT) != 0;
	currentMouseButtons_[2] = (mouseInput & MOUSE_INPUT_MIDDLE) != 0;

	mouseWheelRotation_ = GetMouseWheelRotVol();
}

bool InputManager::IsKeyPressed(int keyCode) const
{
	if (keyCode < 0 || keyCode >= 256) return false;
	return currentKeys_[keyCode] != 0;
}

bool InputManager::IsKeyTriggered(int keyCode) const
{
	if (keyCode < 0 || keyCode >= 256) return false;
	return currentKeys_[keyCode] != 0 && prevKeys_[keyCode] == 0;
}

bool InputManager::IsKeyReleased(int keyCode) const
{
	if (keyCode < 0 || keyCode >= 256) return false;
	return currentKeys_[keyCode] == 0 && prevKeys_[keyCode] != 0;
}

Vector2 InputManager::GetMousePosition() const
{
	return mousePos_;
}

Vector2 InputManager::GetMouseDelta() const
{
	return mousePos_ - prevMousePos_;
}

bool InputManager::IsMousePressed(int button) const
{
	if (button < 0 || button >= 3) return false;
	return currentMouseButtons_[button];
}

bool InputManager::IsMouseTriggered(int button) const
{
	if (button < 0 || button >= 3) return false;
	return currentMouseButtons_[button] && !prevMouseButtons_[button];
}

bool InputManager::IsMouseReleased(int button) const
{
	if (button < 0 || button >= 3) return false;
	return !currentMouseButtons_[button] && prevMouseButtons_[button];
}

int InputManager::GetMouseWheelRotation() const
{
	return mouseWheelRotation_;
}

Vector2 InputManager::GetDirectionInput() const
{
	float dx = 0.0f;
	float dy = 0.0f;

	if (currentKeys_[KEY_INPUT_W] != 0 || currentKeys_[KEY_INPUT_UP] != 0) dy -= 1.0f;
	if (currentKeys_[KEY_INPUT_S] != 0 || currentKeys_[KEY_INPUT_DOWN] != 0) dy += 1.0f;
	if (currentKeys_[KEY_INPUT_A] != 0 || currentKeys_[KEY_INPUT_LEFT] != 0) dx -= 1.0f;
	if (currentKeys_[KEY_INPUT_D] != 0 || currentKeys_[KEY_INPUT_RIGHT] != 0) dx += 1.0f;

	Vector2 dir(dx, dy);
	if (dir.LengthSquared() > 0.0f)
	{
		return dir.Normalized();
	}
	return Vector2::Zero;
}

bool InputManager::IsConfirmTriggered() const
{
	return IsKeyTriggered(KEY_INPUT_RETURN) || IsKeyTriggered(KEY_INPUT_SPACE);
}

bool InputManager::IsCancelTriggered() const
{
	return IsKeyTriggered(KEY_INPUT_ESCAPE) || IsKeyTriggered(KEY_INPUT_BACK);
}
