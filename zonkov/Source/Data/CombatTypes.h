#pragma once
#include <cstdint>

// 装備スロット
enum class EquipSlot : uint8_t
{
	Weapon,
	Shield,
	Armor,
	Backpack,
	Rig,
	Count
};

// 攻撃属性
enum class AttackType : uint8_t
{
	None = 0,
	Slash,   // 斬撃 -> 出血
	Pierce,  // 刺突 -> 骨折
	Blunt,   // 打撃 -> 打撲
	Count
};

// 状態異常タイプ（Phase 4で使用）
enum class AilmentType : uint8_t
{
	None = 0,
	Bleeding,  // 出血（斬撃）
	Fracture,  // 骨折（刺突）
	Bruise,    // 打撲（打撃）
	Count
};

// 攻撃属性から状態異常を取得
inline AilmentType GetAilmentFromAttackType(AttackType type)
{
	switch (type)
	{
	case AttackType::Slash:  return AilmentType::Bleeding;
	case AttackType::Pierce: return AilmentType::Fracture;
	case AttackType::Blunt:  return AilmentType::Bruise;
	default: return AilmentType::None;
	}
}

// ダメージ計算結果
struct DamageResult
{
	int baseDamage;           // 基本ダメージ
	int finalDamage;          // 最終ダメージ
	AttackType originalType;  // 元の攻撃属性
	AttackType effectiveType; // 盾変換後の属性
	float ailmentChance;      // 状態異常発生確率

	DamageResult()
		: baseDamage(0)
		, finalDamage(0)
		, originalType(AttackType::None)
		, effectiveType(AttackType::None)
		, ailmentChance(0.0f)
	{}
};
