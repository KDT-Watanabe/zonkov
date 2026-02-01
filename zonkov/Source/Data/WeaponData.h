#pragma once
#include "CombatTypes.h"

// 武器カテゴリ
enum class WeaponCategory
{
	Melee,   // 近接武器
	Bow,     // 弓
	Magic,   // 魔法
	Count
};

// 武器の戦闘データ
struct WeaponData
{
	int itemId;            // ItemDataへのリンク
	AttackType attackType; // 攻撃属性
	int baseDamage;        // 基本ダメージ
	float attackSpeed;     // 攻撃速度（秒間攻撃回数）
	float range;           // 攻撃範囲（ピクセル）
	float ailmentChance;   // 状態異常発生確率（0.0-1.0）

	// 遠距離武器用
	WeaponCategory category;  // 武器カテゴリ
	float projectileSpeed;    // 弾速（遠距離武器のみ）
	int ammoItemId;           // 必要な弾薬のアイテムID（0なら不要）
	int ammoPerShot;          // 1発あたりの弾薬消費量

	WeaponData()
		: itemId(0)
		, attackType(AttackType::Slash)
		, baseDamage(10)
		, attackSpeed(1.0f)
		, range(40.0f)
		, ailmentChance(0.15f)
		, category(WeaponCategory::Melee)
		, projectileSpeed(0.0f)
		, ammoItemId(0)
		, ammoPerShot(0)
	{}

	WeaponData(int id, AttackType type, int damage, float speed, float r, float ailment = 0.15f)
		: itemId(id)
		, attackType(type)
		, baseDamage(damage)
		, attackSpeed(speed)
		, range(r)
		, ailmentChance(ailment)
		, category(WeaponCategory::Melee)
		, projectileSpeed(0.0f)
		, ammoItemId(0)
		, ammoPerShot(0)
	{}

	bool IsRanged() const { return category != WeaponCategory::Melee; }
	bool RequiresAmmo() const { return ammoItemId != 0; }
};
