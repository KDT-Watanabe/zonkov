#pragma once
#include "CombatTypes.h"

// 属性別ダメージ軽減率
struct DamageReduction
{
	float slash;   // 斬撃軽減率（0.0-1.0）
	float pierce;  // 刺突軽減率
	float blunt;   // 打撃軽減率

	DamageReduction()
		: slash(0.0f), pierce(0.0f), blunt(0.0f)
	{}

	DamageReduction(float s, float p, float b)
		: slash(s), pierce(p), blunt(b)
	{}

	float GetReduction(AttackType type) const
	{
		switch (type)
		{
		case AttackType::Slash:  return slash;
		case AttackType::Pierce: return pierce;
		case AttackType::Blunt:  return blunt;
		default: return 0.0f;
		}
	}
};

// 盾の属性変換ルール
struct AttributeConversion
{
	AttackType fromType;    // 変換元の属性
	AttackType toType;      // 変換先の属性
	float conversionRate;   // 変換率（0.0-1.0）

	AttributeConversion()
		: fromType(AttackType::None)
		, toType(AttackType::None)
		, conversionRate(0.0f)
	{}

	AttributeConversion(AttackType from, AttackType to, float rate)
		: fromType(from)
		, toType(to)
		, conversionRate(rate)
	{}
};

// 防具・盾の戦闘データ
struct ArmorData
{
	int itemId;                      // ItemDataへのリンク
	bool isShield;                   // 盾かどうか
	int baseDefense;                 // 基本防御力
	DamageReduction resistances;     // 属性別軽減率
	AttributeConversion conversion;  // 盾の属性変換
	float blockChance;               // ブロック確率（盾のみ）

	ArmorData()
		: itemId(0)
		, isShield(false)
		, baseDefense(0)
		, resistances()
		, conversion()
		, blockChance(0.0f)
	{}

	ArmorData(int id, bool shield, int defense)
		: itemId(id)
		, isShield(shield)
		, baseDefense(defense)
		, resistances()
		, conversion()
		, blockChance(0.0f)
	{}
};
