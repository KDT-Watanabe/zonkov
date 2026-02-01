#pragma once
#include "Utility/Vector2.h"
#include "Data/CombatTypes.h"
#include <vector>

// 弾種
enum class ProjectileType
{
	Arrow,      // 弓矢
	MagicBolt,  // 魔法弾
	Fireball,   // 火球
	Count
};

// 発射体データ
struct ProjectileData
{
	int damage;
	float speed;
	float range;
	float radius;
	AttackType attackType;
	bool piercing;  // 貫通するか

	ProjectileData()
		: damage(10)
		, speed(400.0f)
		, range(300.0f)
		, radius(5.0f)
		, attackType(AttackType::Pierce)
		, piercing(false)
	{}
};

// 発射体クラス
class Projectile
{
public:
	Projectile(ProjectileType type, const Vector2& startPos, const Vector2& direction,
		int damage, float speed, float range, bool isPlayerOwned);
	~Projectile() = default;

	void Update(float deltaTime);
	void Draw(const Vector2& cameraOffset) const;

	bool IsActive() const { return isActive_; }
	void Deactivate() { isActive_ = false; }

	const Vector2& GetPosition() const { return position_; }
	float GetRadius() const { return radius_; }
	int GetDamage() const { return damage_; }
	AttackType GetAttackType() const { return attackType_; }
	bool IsPlayerOwned() const { return isPlayerOwned_; }
	bool IsPiercing() const { return piercing_; }
	ProjectileType GetType() const { return type_; }

	// ヒット済みターゲット管理（貫通弾用）
	void AddHitTarget(void* target) { hitTargets_.push_back(target); }
	bool HasHitTarget(void* target) const;

private:
	ProjectileType type_;
	Vector2 position_;
	Vector2 direction_;
	Vector2 startPosition_;
	float speed_;
	float range_;
	float radius_;
	int damage_;
	AttackType attackType_;
	bool isActive_;
	bool isPlayerOwned_;
	bool piercing_;
	float distanceTraveled_;

	std::vector<void*> hitTargets_;  // 貫通弾のヒット済みターゲット
};
