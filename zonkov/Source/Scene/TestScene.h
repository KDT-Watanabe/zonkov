#pragma once
#include "IScene.h"
#include "Utility/Vector2.h"
#include <memory>

class TestScene : public IScene
{
public:
	TestScene() = default;
	~TestScene() override = default;

	void Init() override;
	void Update() override;
	void Draw() override;
	std::unique_ptr<IScene> GetNextScene() override;

private:
	Vector2 playerPos_;
	float playerSpeed_ = 0.0f;
	Vector2 mousePos_;
	bool leftClick_ = false;
	bool rightClick_ = false;
};
