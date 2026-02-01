#pragma once
#include "Utility/Vector2.h"
#include "System/Inventory/Inventory.h"
#include "Data/LootTable.h"
#include <random>

// コンテナの種類
enum class ContainerType
{
	Crate,       // 木箱
	Barrel,      // 樽
	WeaponCrate, // 武器箱
	Treasure,    // 宝箱
	Corpse,      // 敵の死体
	GearCrate,   // ギア箱（バックパック・リグ）
	RangedCrate, // 遠距離武器箱
	ArmorCrate,  // 防具箱
	MedicalCrate,// 医療品箱
	Count
};

// ルートコンテナ
class LootContainer
{
public:
	LootContainer(ContainerType type, const Vector2& pos);
	~LootContainer() = default;

	// 中身を生成
	void GenerateLoot(std::mt19937& rng);
	void GenerateLoot(const LootTable& table, std::mt19937& rng);

	// 描画
	void Draw(const Vector2& cameraOffset) const;

	// インタラクション
	bool IsInRange(const Vector2& playerPos, float playerRadius) const;
	bool IsLooted() const { return isLooted_; }
	bool IsEmpty() const;
	void MarkAsLooted() { isLooted_ = true; }

	// プロパティ
	ContainerType GetType() const { return type_; }
	const Vector2& GetPosition() const { return position_; }
	float GetInteractRadius() const { return interactRadius_; }

	// インベントリアクセス
	Inventory& GetInventory() { return contents_; }
	const Inventory& GetInventory() const { return contents_; }

private:
	ContainerType type_;
	Vector2 position_;
	float interactRadius_;
	bool isLooted_;
	Inventory contents_;  // 小さいインベントリ（4x3）
};
