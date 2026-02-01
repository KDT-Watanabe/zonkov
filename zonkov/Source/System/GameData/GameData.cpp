#include "GameData.h"
#include "Data/ItemDatabase.h"

void GameData::Initialize()
{
	if (isInitialized_) return;
	isInitialized_ = true;

	money_ = 1000;  // 初期所持金

	// スタッシュとレイドインベントリを初期化
	// スタッシュは大きめサイズ（10x10）
	stash_ = Inventory(10, 10);
	raidInventory_ = Inventory();

	// 装備初期化
	for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i)
	{
		equippedItems_[i] = 0;
	}

	// 初期アイテムをスタッシュに追加
	stash_.AddItem(1, 3);   // Small Potion x3
	stash_.AddItem(3, 5);   // Bandage x5
	stash_.AddItem(12, 1);  // Dagger
}

bool GameData::SpendMoney(int amount)
{
	if (money_ < amount) return false;
	money_ -= amount;
	return true;
}

void GameData::OnRaidSuccess()
{
	// レイドインベントリの中身をスタッシュに移動
	for (const auto& slot : raidInventory_.GetSlots())
	{
		if (slot.IsEmpty()) continue;
		stash_.AddItem(slot.itemId, slot.stackCount);
	}
	raidInventory_ = Inventory();  // クリア
}

void GameData::OnRaidFailed()
{
	// 失敗時はレイドインベントリを失う
	raidInventory_ = Inventory();
}

void GameData::PrepareForRaid()
{
	// レイドインベントリをクリア（出撃前に装備を選ぶ）
	raidInventory_ = Inventory();
}

int GameData::GetEquippedItem(EquipSlot slot) const
{
	int index = static_cast<int>(slot);
	if (index >= 0 && index < static_cast<int>(EquipSlot::Count))
	{
		return equippedItems_[index];
	}
	return 0;
}

void GameData::SetEquippedItem(EquipSlot slot, int itemId)
{
	int index = static_cast<int>(slot);
	if (index >= 0 && index < static_cast<int>(EquipSlot::Count))
	{
		equippedItems_[index] = itemId;
	}
}

void GameData::UnequipItem(EquipSlot slot)
{
	SetEquippedItem(slot, 0);
}

bool GameData::HasEquippedItem(EquipSlot slot) const
{
	return GetEquippedItem(slot) != 0;
}
