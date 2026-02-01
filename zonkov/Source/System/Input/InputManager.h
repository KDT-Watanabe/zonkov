#pragma once
#include "Utility/Vector2.h"

class InputManager
{
public:
	static InputManager& GetInstance()
	{
		static InputManager instance;
		return instance;
	}

	void Update();

	bool IsKeyPressed(int keyCode) const;
	bool IsKeyTriggered(int keyCode) const;
	bool IsKeyReleased(int keyCode) const;

	Vector2 GetMousePosition() const;
	Vector2 GetMouseDelta() const;
	bool IsMousePressed(int button) const;
	bool IsMouseTriggered(int button) const;
	bool IsMouseReleased(int button) const;
	int GetMouseWheelRotation() const;

	Vector2 GetDirectionInput() const;
	bool IsConfirmTriggered() const;
	bool IsCancelTriggered() const;

private:
	InputManager();
	~InputManager() = default;
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	char currentKeys_[256];
	char prevKeys_[256];

	Vector2 mousePos_;
	Vector2 prevMousePos_;
	bool currentMouseButtons_[3];
	bool prevMouseButtons_[3];
	int mouseWheelRotation_;
};
