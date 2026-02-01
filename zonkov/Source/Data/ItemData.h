#pragma once
#include <string>

// アイテムの種類
enum class ItemType
{
	Consumable,   // 消費アイテム（回復薬など）
	Weapon,       // 武器
	Armor,        // 防具
	Backpack,     // バックパック（容量増加）
	Rig,          // リグ（クイックスロット）
	Material,     // 素材
	Valuable,     // 貴重品（売却用）

	Count
};

// アイテム定義データ
struct ItemData
{
	int id;                // アイテムID
	std::string name;      // 名前
	std::string description;  // 説明
	ItemType type;         // 種類
	int gridWidth;         // インベントリでの横サイズ
	int gridHeight;        // インベントリでの縦サイズ
	float weight;          // 重量
	int maxStack;          // 最大スタック数（1なら重ね不可）
	int graphHandle;       // 画像ハンドル（-1ならダミー描画）
	int healAmount;        // HP回復量（消費アイテム用、0なら回復なし）

	ItemData()
		: id(0)
		, name("")
		, description("")
		, type(ItemType::Material)
		, gridWidth(1)
		, gridHeight(1)
		, weight(0.1f)
		, maxStack(1)
		, graphHandle(-1)
		, healAmount(0)
	{}

	ItemData(int id, const std::string& name, ItemType type,
		int gridW, int gridH, float weight, int maxStack = 1,
		const std::string& desc = "", int heal = 0)
		: id(id)
		, name(name)
		, description(desc)
		, type(type)
		, gridWidth(gridW)
		, gridHeight(gridH)
		, weight(weight)
		, maxStack(maxStack)
		, graphHandle(-1)
		, healAmount(heal)
	{}
};
