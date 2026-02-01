#include "DxLib.h"
#include "MapSystem.h"
#include "System/Resource/ResourceManager.h"

MapSystem::MapSystem()
	: mapWidth_(800.0f)
	, mapHeight_(600.0f)
	, playerStartPos_(100.0f, 300.0f)
{
}

void MapSystem::LoadTestMap()
{
	walls_.clear();

	// マップサイズ（4倍に拡大: 800x600 -> 3200x2400）
	mapWidth_ = 3200.0f;
	mapHeight_ = 2400.0f;

	// 外周の壁
	const float wallThickness = 32.0f;
	walls_.emplace_back(0.0f, 0.0f, mapWidth_, wallThickness);                    // 上
	walls_.emplace_back(0.0f, mapHeight_ - wallThickness, mapWidth_, wallThickness); // 下
	walls_.emplace_back(0.0f, 0.0f, wallThickness, mapHeight_);                    // 左
	walls_.emplace_back(mapWidth_ - wallThickness, 0.0f, wallThickness, mapHeight_); // 右

	// エリア1: 左上付近（スタート地点周辺）
	walls_.emplace_back(200.0f, 150.0f, 200.0f, 32.0f);   // 横壁
	walls_.emplace_back(200.0f, 150.0f, 32.0f, 250.0f);   // 縦壁

	// エリア2: 中央左
	walls_.emplace_back(400.0f, 600.0f, 32.0f, 300.0f);   // 縦壁
	walls_.emplace_back(400.0f, 600.0f, 250.0f, 32.0f);   // 横壁
	walls_.emplace_back(600.0f, 600.0f, 32.0f, 200.0f);   // 縦壁

	// エリア3: 上部中央
	walls_.emplace_back(800.0f, 200.0f, 400.0f, 32.0f);   // 長い横壁
	walls_.emplace_back(1000.0f, 200.0f, 32.0f, 300.0f);  // 縦壁

	// エリア4: 中央
	walls_.emplace_back(1400.0f, 800.0f, 32.0f, 400.0f);  // 長い縦壁
	walls_.emplace_back(1400.0f, 800.0f, 300.0f, 32.0f);  // 横壁
	walls_.emplace_back(1400.0f, 1168.0f, 300.0f, 32.0f); // 横壁（下）

	// エリア5: 右上
	walls_.emplace_back(2200.0f, 300.0f, 32.0f, 350.0f);  // 縦壁
	walls_.emplace_back(2200.0f, 300.0f, 400.0f, 32.0f);  // 横壁
	walls_.emplace_back(2560.0f, 300.0f, 32.0f, 250.0f);  // 縦壁

	// エリア6: 下部中央
	walls_.emplace_back(800.0f, 1600.0f, 32.0f, 400.0f);  // 縦壁
	walls_.emplace_back(800.0f, 1600.0f, 300.0f, 32.0f);  // 横壁
	walls_.emplace_back(800.0f, 1968.0f, 300.0f, 32.0f);  // 横壁（下）

	// エリア7: 中央右
	walls_.emplace_back(2000.0f, 1200.0f, 400.0f, 32.0f);  // 横壁
	walls_.emplace_back(2000.0f, 1200.0f, 32.0f, 300.0f);  // 縦壁
	walls_.emplace_back(2360.0f, 1200.0f, 32.0f, 300.0f);  // 縦壁

	// エリア8: 右下（出口付近）
	walls_.emplace_back(2600.0f, 1800.0f, 32.0f, 350.0f);  // 縦壁
	walls_.emplace_back(2600.0f, 1800.0f, 300.0f, 32.0f);  // 横壁

	// エリア9: 左下
	walls_.emplace_back(300.0f, 1800.0f, 250.0f, 32.0f);  // 横壁
	walls_.emplace_back(300.0f, 1800.0f, 32.0f, 300.0f);  // 縦壁

	// 脱出エリア（右下付近）
	exitArea_ = RectF(3050.0f, 2250.0f, 100.0f, 100.0f);

	// プレイヤー開始位置（左上付近）
	playerStartPos_ = Vector2(150.0f, 150.0f);
}

void MapSystem::Draw(const Vector2& cameraOffset) const
{
	int ox = static_cast<int>(cameraOffset.x);
	int oy = static_cast<int>(cameraOffset.y);

	auto& res = ResourceManager::GetInstance();
	int floorHandle = res.GetGraph("floor");
	int wallHandle = res.GetGraph("wall");
	int exitHandle = res.GetGraph("exit_area");

	// 背景（床）
	if (floorHandle != -1)
	{
		// タイル描画
		int tileW, tileH;
		GetGraphSize(floorHandle, &tileW, &tileH);
		if (tileW > 0 && tileH > 0)
		{
			int startX = (ox / tileW) * tileW;
			int startY = (oy / tileH) * tileH;
			for (int y = startY; y < oy + 720 + tileH && y < static_cast<int>(mapHeight_); y += tileH)
			{
				for (int x = startX; x < ox + 1280 + tileW && x < static_cast<int>(mapWidth_); x += tileW)
				{
					DrawGraph(x - ox, y - oy, floorHandle, TRUE);
				}
			}
		}
	}
	else
	{
		// フォールバック
		DrawBox(0 - ox, 0 - oy,
			static_cast<int>(mapWidth_) - ox, static_cast<int>(mapHeight_) - oy,
			GetColor(50, 50, 60), TRUE);
	}

	// 壁の描画
	for (const auto& wall : walls_)
	{
		int wallX = static_cast<int>(wall.rect.x) - ox;
		int wallY = static_cast<int>(wall.rect.y) - oy;
		int wallW = static_cast<int>(wall.rect.width);
		int wallH = static_cast<int>(wall.rect.height);

		if (wall.graphHandle != -1)
		{
			DrawGraph(wallX, wallY, wall.graphHandle, TRUE);
		}
		else if (wallHandle != -1)
		{
			// タイル描画
			int tileW, tileH;
			GetGraphSize(wallHandle, &tileW, &tileH);
			if (tileW > 0 && tileH > 0)
			{
				for (int ty = 0; ty < wallH; ty += tileH)
				{
					for (int tx = 0; tx < wallW; tx += tileW)
					{
						DrawGraph(wallX + tx, wallY + ty, wallHandle, TRUE);
					}
				}
			}
		}
		else
		{
			// フォールバック
			DrawBox(wallX, wallY, wallX + wallW, wallY + wallH,
				GetColor(100, 100, 110), TRUE);
		}
	}

	// 脱出エリアの描画
	int exitX = static_cast<int>(exitArea_.x) - ox;
	int exitY = static_cast<int>(exitArea_.y) - oy;
	int exitW = static_cast<int>(exitArea_.width);
	int exitH = static_cast<int>(exitArea_.height);

	if (exitHandle != -1)
	{
		DrawExtendGraph(exitX, exitY, exitX + exitW, exitY + exitH, exitHandle, TRUE);
	}
	else
	{
		// フォールバック
		DrawBox(exitX, exitY, exitX + exitW, exitY + exitH,
			GetColor(50, 150, 50), TRUE);
		DrawString(exitX + 5, exitY + 20, "出口", GetColor(255, 255, 255));
	}
}

bool MapSystem::IsCollidingWithWall(const RectF& rect) const
{
	for (const auto& wall : walls_)
	{
		if (wall.rect.Intersects(rect))
		{
			return true;
		}
	}
	return false;
}

bool MapSystem::IsCollidingWithWall(const Vector2& pos, float radius) const
{
	for (const auto& wall : walls_)
	{
		if (wall.rect.IntersectsCircle(pos, radius))
		{
			return true;
		}
	}
	return false;
}

bool MapSystem::IsInExitArea(const Vector2& pos) const
{
	return exitArea_.Contains(pos);
}
