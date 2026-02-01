#include "ShopSystem.h"
#include "System/GameData/GameData.h"
#include "Data/ItemDatabase.h"

void ShopSystem::Initialize()
{
	if (isInitialized_) return;
	isInitialized_ = true;

	items_.clear();

	// 商品リスト（itemId, 購入価格, 売却価格, 在庫）
	// 消費アイテム
	items_.emplace_back(1, 50, 20, -1);    // Small Potion
	items_.emplace_back(2, 120, 50, -1);   // Large Potion
	items_.emplace_back(3, 30, 10, -1);    // Bandage

	// 武器
	items_.emplace_back(10, 300, 120, 3);  // Short Sword
	items_.emplace_back(11, 500, 200, 2);  // Long Sword
	items_.emplace_back(12, 150, 60, 5);   // Dagger

	// 防具
	items_.emplace_back(20, 400, 160, 2);  // Leather Armor
	items_.emplace_back(21, 350, 140, 3);  // Iron Shield

	// 素材（売却のみ、購入不可）
	items_.emplace_back(30, 0, 15, 0);     // Iron Ore（購入不可）
	items_.emplace_back(31, 0, 5, 0);      // Herb（購入不可）

	// 貴重品（売却のみ）
	items_.emplace_back(40, 0, 100, 0);    // Gold Ring
	items_.emplace_back(41, 0, 50, 0);     // Ancient Coin
}

bool ShopSystem::BuyItem(int itemId, int count)
{
	ShopItem* item = FindShopItem(itemId);
	if (!item) return false;

	// 購入価格が0なら購入不可
	if (item->buyPrice <= 0) return false;

	// 在庫チェック
	if (item->stock == 0) return false;
	if (item->stock > 0 && item->stock < count) return false;

	// 合計金額
	int totalPrice = item->buyPrice * count;

	// 所持金チェック
	GameData& gameData = GameData::GetInstance();
	if (gameData.GetMoney() < totalPrice) return false;

	// スタッシュに空きがあるかチェック
	if (!gameData.GetStash().CanFitItem(itemId)) return false;

	// 購入実行
	if (!gameData.SpendMoney(totalPrice)) return false;
	if (!gameData.GetStash().AddItem(itemId, count))
	{
		// 追加失敗時は返金
		gameData.AddMoney(totalPrice);
		return false;
	}

	// 在庫を減らす
	if (item->stock > 0)
	{
		item->stock -= count;
	}

	return true;
}

int ShopSystem::GetSellPrice(int itemId) const
{
	const ShopItem* item = FindShopItem(itemId);
	if (!item) return 0;
	return item->sellPrice;
}

bool ShopSystem::SellItem(int itemId, int count)
{
	const ShopItem* item = FindShopItem(itemId);
	if (!item) return false;

	int totalPrice = item->sellPrice * count;

	// スタッシュからアイテムを探して削除
	GameData& gameData = GameData::GetInstance();
	Inventory& stash = gameData.GetStash();

	int remaining = count;
	for (int i = 0; i < static_cast<int>(stash.GetSlots().size()) && remaining > 0; ++i)
	{
		const InventorySlot* slot = stash.GetSlot(i);
		if (!slot || slot->IsEmpty() || slot->itemId != itemId) continue;

		int toRemove = (slot->stackCount < remaining) ? slot->stackCount : remaining;
		stash.RemoveItem(i, toRemove);
		remaining -= toRemove;
	}

	if (remaining > 0)
	{
		// 売却に必要な数が足りなかった（本来はここには来ない）
		return false;
	}

	// お金を追加
	gameData.AddMoney(totalPrice);
	return true;
}

ShopItem* ShopSystem::FindShopItem(int itemId)
{
	for (auto& item : items_)
	{
		if (item.itemId == itemId) return &item;
	}
	return nullptr;
}

const ShopItem* ShopSystem::FindShopItem(int itemId) const
{
	for (const auto& item : items_)
	{
		if (item.itemId == itemId) return &item;
	}
	return nullptr;
}
