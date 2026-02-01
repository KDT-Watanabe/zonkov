#pragma once
#include "System/Inventory/Inventory.h"
#include "Data/CombatTypes.h"

// ゲーム全体のデータ管理（シングルトン）
class GameData
{
public:
	static GameData& GetInstance()
	{
		static GameData instance;
		return instance;
	}

	// 初期化
	void Initialize();

	// お金
	int GetMoney() const { return money_; }
	void AddMoney(int amount) { money_ += amount; }
	bool SpendMoney(int amount);

	// スタッシュ（倉庫）
	Inventory& GetStash() { return stash_; }
	const Inventory& GetStash() const { return stash_; }

	// レイド用インベントリ（出撃時に持ち込むアイテム）
	Inventory& GetRaidInventory() { return raidInventory_; }
	const Inventory& GetRaidInventory() const { return raidInventory_; }

	// レイド結果を反映（持ち帰りアイテムをスタッシュに移動）
	void OnRaidSuccess();
	void OnRaidFailed();

	// レイド準備（スタッシュからレイドインベントリへ）
	void PrepareForRaid();

	// 装備
	int GetEquippedItem(EquipSlot slot) const;
	void SetEquippedItem(EquipSlot slot, int itemId);
	void UnequipItem(EquipSlot slot);
	bool HasEquippedItem(EquipSlot slot) const;

private:
	GameData() : money_(0), isInitialized_(false) {}
	~GameData() = default;
	GameData(const GameData&) = delete;
	GameData& operator=(const GameData&) = delete;

	bool isInitialized_;
	int money_;
	Inventory stash_;          // 倉庫（大きめ）
	Inventory raidInventory_;  // レイド用
	int equippedItems_[static_cast<int>(EquipSlot::Count)];  // 装備
};
