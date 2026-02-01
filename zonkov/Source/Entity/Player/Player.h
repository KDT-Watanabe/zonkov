#pragma once
#include "Entity/Base/Actor.h"
#include "Data/CombatTypes.h"

class MapSystem;

class Player : public Actor
{
public:
	Player();
	~Player() override = default;

	void Update(float deltaTime) override;
	void Draw(const Vector2& cameraOffset) const override;

	// 移動
	void UpdateMovement(float deltaTime, const MapSystem& map, float weightRatio);
	Vector2 TryMove(const Vector2& currentPos, const Vector2& delta, const MapSystem& map) const;

	// 戦闘
	void Attack();
	bool CanAttack() const;
	bool IsAttacking() const { return isAttacking_; }
	float GetAttackTimer() const { return attackTimer_; }
	Vector2 GetFacingDirection() const { return facingDirection_; }
	void SetFacingDirection(const Vector2& dir);
	void FaceTowards(const Vector2& targetPos);

	// 装備
	void Equip(EquipSlot slot, int itemId);
	void Unequip(EquipSlot slot);
	int GetEquippedItemId(EquipSlot slot) const;
	bool HasEquipped(EquipSlot slot) const;

	// 戦闘ステータス（装備から計算）
	AttackType GetAttackType() const;
	int GetAttackDamage() const;
	float GetAttackRange() const;
	float GetAttackSpeed() const;

	// 速度
	float GetBaseSpeed() const { return baseSpeed_; }
	void SetBaseSpeed(float speed) { baseSpeed_ = speed; }

	// ダメージ・無敵
	void TakeDamage(int amount);
	bool IsInvincible() const { return invincibilityTimer_ > 0.0f; }
	float GetInvincibilityTimer() const { return invincibilityTimer_; }

	// 無敵時間定数
	static constexpr float INVINCIBILITY_DURATION = 1.0f;

private:
	void UpdateCombat(float deltaTime);
	void UpdateFacingDirection();

private:
	// 戦闘状態
	float attackCooldown_;  // 攻撃間隔
	float attackTimer_;     // 現在のクールダウン
	bool isAttacking_;
	float attackDuration_;  // 攻撃アニメーション時間

	// 移動
	float baseSpeed_;
	Vector2 facingDirection_;
	Vector2 lastMoveDirection_;

	// 無敵時間
	float invincibilityTimer_;
};
