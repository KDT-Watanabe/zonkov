#include "Projectile.h"
#include <DxLib.h>

Projectile::Projectile(ProjectileType type, const Vector2& startPos, const Vector2& direction,
	int damage, float speed, float range, bool isPlayerOwned)
	: type_(type)
	, position_(startPos)
	, direction_(direction.Normalized())
	, startPosition_(startPos)
	, speed_(speed)
	, range_(range)
	, radius_(5.0f)
	, damage_(damage)
	, attackType_(AttackType::Pierce)
	, isActive_(true)
	, isPlayerOwned_(isPlayerOwned)
	, piercing_(false)
	, distanceTraveled_(0.0f)
{
	// タイプに応じて属性を設定
	switch (type)
	{
	case ProjectileType::Arrow:
		attackType_ = AttackType::Pierce;
		radius_ = 4.0f;
		break;
	case ProjectileType::MagicBolt:
		attackType_ = AttackType::Blunt;
		radius_ = 6.0f;
		break;
	case ProjectileType::Fireball:
		attackType_ = AttackType::Blunt;
		radius_ = 10.0f;
		piercing_ = true;  // ファイアボールは貫通
		break;
	default:
		break;
	}
}

void Projectile::Update(float deltaTime)
{
	if (!isActive_) return;

	// 移動
	Vector2 delta = direction_ * speed_ * deltaTime;
	position_ = position_ + delta;
	distanceTraveled_ += delta.GetLength();

	// 射程チェック
	if (distanceTraveled_ >= range_)
	{
		isActive_ = false;
	}
}

void Projectile::Draw(const Vector2& cameraOffset) const
{
	if (!isActive_) return;

	int ox = static_cast<int>(cameraOffset.x);
	int oy = static_cast<int>(cameraOffset.y);
	int px = static_cast<int>(position_.x) - ox;
	int py = static_cast<int>(position_.y) - oy;
	int r = static_cast<int>(radius_);

	unsigned int color;
	switch (type_)
	{
	case ProjectileType::Arrow:
		color = GetColor(139, 90, 43);  // 茶色
		// 矢のライン描画
		{
			int lineLen = 15;
			int endX = px - static_cast<int>(direction_.x * lineLen);
			int endY = py - static_cast<int>(direction_.y * lineLen);
			DrawLine(px, py, endX, endY, color, 2);
			DrawCircle(px, py, 2, GetColor(100, 100, 100), TRUE);
		}
		break;
	case ProjectileType::MagicBolt:
		color = GetColor(100, 150, 255);  // 青
		DrawCircle(px, py, r, color, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
		DrawCircle(px, py, r + 3, color, FALSE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		break;
	case ProjectileType::Fireball:
		color = GetColor(255, 100, 50);  // オレンジ
		DrawCircle(px, py, r, color, TRUE);
		DrawCircle(px, py, r - 2, GetColor(255, 200, 100), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
		DrawCircle(px, py, r + 5, GetColor(255, 50, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		break;
	default:
		color = GetColor(255, 255, 255);
		DrawCircle(px, py, r, color, TRUE);
		break;
	}
}

bool Projectile::HasHitTarget(void* target) const
{
	for (const auto& t : hitTargets_)
	{
		if (t == target) return true;
	}
	return false;
}
