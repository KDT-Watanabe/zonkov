#include "DxLib.h"
#include "System/Scene/SceneManager.h"
#include "System/Resource/ResourceManager.h"
#include "Scene/BaseScene.h"
#include <memory>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	ChangeWindowMode(TRUE);
	SetGraphMode(1280, 720, 32);
	SetMainWindowText("Zonkov");

	if (DxLib_Init() == -1)
	{
		return -1;
	}

	SetDrawScreen(DX_SCREEN_BACK);

	// Load all game assets
	ResourceManager::GetInstance().LoadAllAssets();

	auto& sceneManager = SceneManager::GetInstance();
	sceneManager.SetScene(std::make_unique<BaseScene>());

	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
	{
		ClearDrawScreen();

		sceneManager.Update();
		sceneManager.Draw();

		ScreenFlip();
	}

	DxLib_End();

	return 0;
}
