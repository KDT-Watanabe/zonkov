#pragma once
#include <memory>

class IScene
{
public:
	virtual ~IScene() = default;
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual std::unique_ptr<IScene> GetNextScene() = 0;
};
