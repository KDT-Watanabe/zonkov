#include "SceneManager.h"
#include "Scene/IScene.h"

void SceneManager::SetScene(std::unique_ptr<IScene> scene)
{
	nextScene_ = std::move(scene);
}

void SceneManager::Update()
{
	if (nextScene_)
	{
		currentScene_ = std::move(nextScene_);
		nextScene_ = nullptr;
		currentScene_->Init();
	}

	if (currentScene_)
	{
		currentScene_->Update();

		auto next = currentScene_->GetNextScene();
		if (next)
		{
			SetScene(std::move(next));
		}
	}
}

void SceneManager::Draw()
{
	if (currentScene_)
	{
		currentScene_->Draw();
	}
}

bool SceneManager::HasScene() const
{
	return currentScene_ != nullptr;
}
