#include "Player.h"
#include "System/Map/MapSystem.h"
#include "System/Input/InputManager.h"
#include "System/Combat/CombatSystem.h"
#include "System/GameData/GameData.h"
#include "System/Resource/ResourceManager.h"
#include <DxLib.h>

Player::Player()
	: attackCooldown_(0.5f)
	, attackTimer_(0.0f)
	, isAttacking_(false)
	, attackDuration_(0.2f)
	, baseSpeed_(200.0f)
	, facingDirection_(1.0f, 0.0f)
	, lastMoveDirection_(0.0f, 0.0f)
	, invincibilityTimer_(0.0f)
{
	radius_ = 12.0f;
	maxHP_ = 100;
	hp_ = maxHP_;
}

void Player::Update(float deltaTime)
{
	UpdateCombat(deltaTime);

	// 無敵時間の更新
	if (invincibilityTimer_ > 0.0f)
	{
		invincibilityTimer_ -= deltaTime;
		if (invincibilityTimer_ < 0.0f)
		{
			invincibilityTimer_ = 0.0f;
		}
	}
}

void Player::Draw(const Vector2& cameraOffset) const
{
	// 無敵中は点滅（0.1秒ごとに表示/非表示を切り替え）
	if (IsInvincible())
	{
		int blinkPhase = static_cast<int>(invincibilityTimer_ * 10) % 2;
		if (blinkPhase == 0)
		{
			// 非表示フレーム - 半透明で描画
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
		}
	}

	int ox = static_cast<int>(cameraOffset.x);
	int oy = static_cast<int>(cameraOffset.y);
	int px = static_cast<int>(position_.x) - ox;
	int py = static_cast<int>(position_.y) - oy;
	int r = static_cast<int>(radius_);

	auto& res = ResourceManager::GetInstance();
	const char* imgKey = isAttacking_ ? "player_attack" : "player";
	int handle = res.GetGraph(imgKey);

	// 画像がない場合はフォールバックを試す
	if (handle == -1 && isAttacking_)
	{
		handle = res.GetGraph("player");
	}

	if (handle != -1)
	{
		// 画像描画（中心基準）
		int imgW, imgH;
		GetGraphSize(handle, &imgW, &imgH);

		// 向きに応じて反転描画
		if (facingDirection_.x < 0)
		{
			DrawTurnGraph(px - imgW / 2, py - imgH / 2, handle, TRUE);
		}
		else
		{
			DrawGraph(px - imgW / 2, py - imgH / 2, handle, TRUE);
		}
	}
	else
	{
		// フォールバック: デバッグ描画
		unsigned int color = GetColor(0, 200, 100);
		if (isAttacking_)
		{
			color = GetColor(255, 200, 100);
		}
		// 無敵中は白っぽく
		if (IsInvincible())
		{
			color = GetColor(200, 255, 200);
		}

		DrawCircle(px, py, r, color, TRUE);
		DrawCircle(px, py, r, GetColor(0, 150, 75), FALSE);

		// 向き表示
		int dirX = px + static_cast<int>(facingDirection_.x * radius_ * 1.5f);
		int dirY = py + static_cast<int>(facingDirection_.y * radius_ * 1.5f);
		DrawLine(px, py, dirX, dirY, GetColor(255, 255, 255));
	}

	// 無敵の点滅終了時にブレンドモードをリセット
	if (IsInvincible())
	{
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	// 攻撃範囲表示（攻撃中、デバッグ用）
	if (isAttacking_)
	{
		float range = GetAttackRange();
		int attackX = px + static_cast<int>(facingDirection_.x * range * 0.5f);
		int attackY = py + static_cast<int>(facingDirection_.y * range * 0.5f);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
		DrawCircle(attackX, attackY, static_cast<int>(range * 0.5f), GetColor(255, 100, 100), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void Player::UpdateMovement(float deltaTime, const MapSystem& map, float weightRatio)
{
	auto& input = InputManager::GetInstance();

	// 重量による速度倍率
	float speedMult = 1.0f;
	if (weightRatio >= 1.0f)
	{
		speedMult = 0.0f;
	}
	else if (weightRatio >= 0.8f)
	{
		speedMult = 0.3f;
	}
	else if (weightRatio >= 0.6f)
	{
		speedMult = 0.7f;
	}

	Vector2 dir = input.GetDirectionInput();
	if (dir.LengthSquared() > 0.0f)
	{
		lastMoveDirection_ = dir;
		UpdateFacingDirection();
	}

	if (dir.LengthSquared() > 0.0f && speedMult > 0.0f)
	{
		Vector2 delta = dir * baseSpeed_ * speedMult * deltaTime;
		position_ = TryMove(position_, delta, map);
	}
}

Vector2 Player::TryMove(const Vector2& currentPos, const Vector2& delta, const MapSystem& map) const
{
	Vector2 newPos = currentPos;

	// X軸移動
	Vector2 testPosX(currentPos.x + delta.x, currentPos.y);
	if (!map.IsCollidingWithWall(testPosX, radius_))
	{
		newPos.x = testPosX.x;
	}

	// Y軸移動
	Vector2 testPosY(newPos.x, currentPos.y + delta.y);
	if (!map.IsCollidingWithWall(testPosY, radius_))
	{
		newPos.y = testPosY.y;
	}

	return newPos;
}

void Player::Attack()
{
	if (!CanAttack()) return;

	isAttacking_ = true;
	attackTimer_ = attackCooldown_;
}

bool Player::CanAttack() const
{
	return attackTimer_ <= 0.0f && !isAttacking_;
}

void Player::UpdateCombat(float deltaTime)
{
	// 攻撃クールダウン
	if (attackTimer_ > 0.0f)
	{
		attackTimer_ -= deltaTime;
	}

	// 攻撃アニメーション終了
	if (isAttacking_)
	{
		if (attackTimer_ <= attackCooldown_ - attackDuration_)
		{
			isAttacking_ = false;
		}
	}
}

void Player::UpdateFacingDirection()
{
	if (lastMoveDirection_.LengthSquared() > 0.0f)
	{
		facingDirection_ = lastMoveDirection_.Normalized();
	}
}

void Player::SetFacingDirection(const Vector2& dir)
{
	if (dir.LengthSquared() > 0.0f)
	{
		facingDirection_ = dir.Normalized();
	}
}

void Player::FaceTowards(const Vector2& targetPos)
{
	Vector2 dir = targetPos - position_;
	SetFacingDirection(dir);
}

void Player::Equip(EquipSlot slot, int itemId)
{
	GameData::GetInstance().SetEquippedItem(slot, itemId);
}

void Player::Unequip(EquipSlot slot)
{
	GameData::GetInstance().UnequipItem(slot);
}

int Player::GetEquippedItemId(EquipSlot slot) const
{
	return GameData::GetInstance().GetEquippedItem(slot);
}

bool Player::HasEquipped(EquipSlot slot) const
{
	return GameData::GetInstance().HasEquippedItem(slot);
}

AttackType Player::GetAttackType() const
{
	int weaponId = GetEquippedItemId(EquipSlot::Weapon);
	if (weaponId == 0)
	{
		return AttackType::Blunt;  // 素手は打撃
	}

	const WeaponData* weapon = CombatSystem::GetInstance().GetWeaponData(weaponId);
	if (weapon)
	{
		return weapon->attackType;
	}
	return AttackType::Blunt;
}

int Player::GetAttackDamage() const
{
	int weaponId = GetEquippedItemId(EquipSlot::Weapon);
	if (weaponId == 0)
	{
		return 5;  // 素手ダメージ
	}

	const WeaponData* weapon = CombatSystem::GetInstance().GetWeaponData(weaponId);
	if (weapon)
	{
		return weapon->baseDamage;
	}
	return 5;
}

float Player::GetAttackRange() const
{
	int weaponId = GetEquippedItemId(EquipSlot::Weapon);
	if (weaponId == 0)
	{
		return 25.0f;  // 素手リーチ
	}

	const WeaponData* weapon = CombatSystem::GetInstance().GetWeaponData(weaponId);
	if (weapon)
	{
		return weapon->range;
	}
	return 25.0f;
}

float Player::GetAttackSpeed() const
{
	int weaponId = GetEquippedItemId(EquipSlot::Weapon);
	if (weaponId == 0)
	{
		return 2.0f;  // 素手は速い
	}

	const WeaponData* weapon = CombatSystem::GetInstance().GetWeaponData(weaponId);
	if (weapon)
	{
		return weapon->attackSpeed;
	}
	return 1.0f;
}

void Player::TakeDamage(int amount)
{
	// 無敵中はダメージを受けない
	if (IsInvincible())
	{
		return;
	}

	// ダメージ適用
	hp_ -= amount;
	if (hp_ < 0)
	{
		hp_ = 0;
	}

	// 無敵時間を開始
	if (hp_ > 0)
	{
		invincibilityTimer_ = INVINCIBILITY_DURATION;
	}
}
