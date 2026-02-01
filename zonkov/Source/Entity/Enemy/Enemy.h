#pragma once
#include "Entity/Base/Actor.h"
#include "Data/CombatTypes.h"
#include <vector>

class MapSystem;
class Player;

// 敵のAI状態
enum class EnemyState : uint8_t
{
	Idle,    // 待機
	Patrol,  // 巡回
	Chase,   // 追跡
	Attack,  // 攻撃
	Count
};

// 敵タイプ
enum class EnemyType : uint8_t
{
	Melee,   // 近接攻撃
	Count
};

class Enemy : public Actor
{
public:
	Enemy(EnemyType type, const Vector2& startPos);
	~Enemy() override = default;

	void Update(float deltaTime) override;
	void Draw(const Vector2& cameraOffset) const override;

	// AI設定
	void SetTarget(const Player* target) { target_ = target; }
	void SetMapSystem(const MapSystem* map) { map_ = map; }
	void SetPatrolPoints(const std::vector<Vector2>& points);

	// 状態取得
	EnemyState GetState() const { return state_; }
	bool IsAggro() const { return state_ == EnemyState::Chase || state_ == EnemyState::Attack; }

	// 戦闘
	AttackType GetAttackType() const { return attackType_; }
	int GetAttackDamage() const { return attackDamage_; }
	float GetAttackRange() const { return attackRange_; }
	bool CanAttack() const;
	bool IsAttacking() const { return isAttacking_; }
	void OnAttackHit();  // 攻撃がヒットした時

	// ダメージ・無敵
	void TakeDamage(int amount);
	bool IsInvincible() const { return invincibilityTimer_ > 0.0f; }
	float GetInvincibilityTimer() const { return invincibilityTimer_; }

	// 無敵時間定数（敵は短め）
	static constexpr float INVINCIBILITY_DURATION = 0.5f;

private:
	// AI状態別更新
	void UpdateIdle(float deltaTime);
	void UpdatePatrol(float deltaTime);
	void UpdateChase(float deltaTime);
	void UpdateAttack(float deltaTime);

	// AI補助
	void TransitionTo(EnemyState newState);
	bool CanSeeTarget() const;
	bool IsTargetInRange() const;
	float GetDistanceToTarget() const;
	Vector2 GetDirectionToTarget() const;
	void MoveToward(const Vector2& target, float deltaTime);
	void FaceTarget();

private:
	EnemyType type_;
	EnemyState state_;
	const Player* target_;
	const MapSystem* map_;

	// 巡回
	std::vector<Vector2> patrolPoints_;
	int currentPatrolIndex_;
	float patrolWaitTimer_;
	float patrolWaitDuration_;

	// 検知
	float detectionRange_;   // プレイヤー発見距離
	float loseAggroRange_;   // アグロ解除距離
	float attackRange_;

	// 戦闘
	AttackType attackType_;
	int attackDamage_;
	float attackCooldown_;
	float attackTimer_;
	bool isAttacking_;
	float attackDuration_;
	bool hasHitThisAttack_;  // この攻撃でヒット済みか

	// 移動
	float moveSpeed_;
	float chaseSpeedMult_;
	Vector2 facingDirection_;

	// 無敵時間
	float invincibilityTimer_;
};
