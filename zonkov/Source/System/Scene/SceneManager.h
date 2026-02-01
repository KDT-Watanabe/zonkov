#pragma once
#include <memory>

class IScene;

class SceneManager
{
public:
	static SceneManager& GetInstance()
	{
		static SceneManager instance;
		return instance;
	}

	void SetScene(std::unique_ptr<IScene> scene);
	void Update();
	void Draw();
	bool HasScene() const;

private:
	SceneManager() = default;
	~SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	std::unique_ptr<IScene> currentScene_;
	std::unique_ptr<IScene> nextScene_;
};
