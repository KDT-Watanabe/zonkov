#pragma once
#include "Utility/Vector2.h"
#include "Utility/RectF.h"
#include <vector>

// 壁オブジェクト（将来的に画像差し替え可能）
struct WallObject
{
	RectF rect;
	int graphHandle;  // -1ならダミー描画

	WallObject(float x, float y, float w, float h)
		: rect(x, y, w, h), graphHandle(-1) {}
};

class MapSystem
{
public:
	MapSystem();
	~MapSystem() = default;

	// テストマップ読み込み
	void LoadTestMap();

	// 更新・描画
	void Draw(const Vector2& cameraOffset) const;	

	// 当たり判定
	bool IsCollidingWithWall(const RectF& rect) const;
	bool IsCollidingWithWall(const Vector2& pos, float radius) const;

	// 脱出エリア判定
	bool IsInExitArea(const Vector2& pos) const;

	// プレイヤー開始位置
	Vector2 GetPlayerStartPosition() const { return playerStartPos_; }

	// マップサイズ
	float GetWidth() const { return mapWidth_; }
	float GetHeight() const { return mapHeight_; }

	// 壁リスト取得（外部からの当たり判定用）
	const std::vector<WallObject>& GetWalls() const { return walls_; }

private:
	std::vector<WallObject> walls_;
	RectF exitArea_;
	Vector2 playerStartPos_;
	float mapWidth_;
	float mapHeight_;
};
