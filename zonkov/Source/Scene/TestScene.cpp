#include "DxLib.h"
#include "TestScene.h"
#include "System/Input/InputManager.h"

void TestScene::Init()
{
	playerPos_ = Vector2(400.0f, 300.0f);
	playerSpeed_ = 200.0f;
	mousePos_ = Vector2::Zero;
	leftClick_ = false;
	rightClick_ = false;
}

void TestScene::Update()
{
	auto& input = InputManager::GetInstance();
	input.Update();

	Vector2 dir = input.GetDirectionInput();
	if (dir.LengthSquared() > 0.0f)
	{
		playerPos_ += dir * playerSpeed_ * (1.0f / 60.0f);
	}

	if (playerPos_.x < 20.0f) playerPos_.x = 20.0f;
	if (playerPos_.x > 780.0f) playerPos_.x = 780.0f;
	if (playerPos_.y < 20.0f) playerPos_.y = 20.0f;
	if (playerPos_.y > 580.0f) playerPos_.y = 580.0f;

	mousePos_ = input.GetMousePosition();
	leftClick_ = input.IsMousePressed(0);
	rightClick_ = input.IsMousePressed(1);
}

void TestScene::Draw()
{
	DrawBox(0, 0, 799, 599, GetColor(30, 30, 40), TRUE);

	int gridColor = GetColor(50, 50, 60);
	for (int x = 0; x < 800; x += 80)
		DrawLine(x, 0, x, 600, gridColor);
	for (int y = 0; y < 600; y += 60)
		DrawLine(0, y, 800, y, gridColor);

	int px = static_cast<int>(playerPos_.x);
	int py = static_cast<int>(playerPos_.y);
	DrawCircle(px, py, 20, GetColor(0, 255, 100), TRUE);
	DrawCircle(px, py, 20, GetColor(0, 200, 80), FALSE);

	if (leftClick_)
		DrawCircle(px, py, 22, GetColor(100, 255, 150), FALSE);
	if (rightClick_)
		DrawCircle(px, py, 24, GetColor(255, 100, 100), FALSE);

	int white = GetColor(255, 255, 255);
	int gray = GetColor(180, 180, 180);

	DrawString(10, 10, "Zonkov - テストシーン", white);
	DrawFormatString(10, 40, gray, "Player: (%d, %d)", px, py);
	DrawFormatString(10, 60, gray, "Mouse:  (%d, %d)", static_cast<int>(mousePos_.x), static_cast<int>(mousePos_.y));
	DrawFormatString(10, 80, gray, "Left: %s  Right: %s",
		leftClick_ ? "ON" : "OFF",
		rightClick_ ? "ON" : "OFF");

	DrawString(10, 560, "WASD/矢印:移動 | ESC:終了", gray);
}

std::unique_ptr<IScene> TestScene::GetNextScene()
{
	return nullptr;
}
