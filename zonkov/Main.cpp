#include "DxLib.h"

// プログラムは WinMain から始まります
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// ウィンドウモード
	ChangeWindowMode(TRUE);

	// ウィンドウサイズ設定
	SetGraphMode(800, 600, 32);

	// ウィンドウタイトル
	SetMainWindowText("Zonkov");

	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
	{
		return -1;			// エラーが起きたら直ちに終了
	}

	// 描画先を裏画面に設定
	SetDrawScreen(DX_SCREEN_BACK);

	// メインループ
	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
	{
		ClearDrawScreen();

		DrawString(10, 10, "Zonkov - Hello DxLib!", GetColor(255, 255, 255));
		DrawString(10, 30, "ESC: Exit", GetColor(200, 200, 200));

		ScreenFlip();
	}

	DxLib_End();				// ＤＸライブラリ使用の終了処理

	return 0;				// ソフトの終了
}