#pragma once
#include <vector>

// ショップの商品
struct ShopItem
{
	int itemId;
	int buyPrice;   // 購入価格
	int sellPrice;  // 売却価格
	int stock;      // 在庫（-1なら無限）

	ShopItem(int id, int buy, int sell, int s = -1)
		: itemId(id), buyPrice(buy), sellPrice(sell), stock(s) {}
};

// ショップシステム
class ShopSystem
{
public:
	static ShopSystem& GetInstance()
	{
		static ShopSystem instance;
		return instance;
	}

	// 初期化（商品リスト設定）
	void Initialize();

	// 商品リスト取得
	const std::vector<ShopItem>& GetShopItems() const { return items_; }

	// 購入
	bool BuyItem(int itemId, int count = 1);

	// 売却価格取得
	int GetSellPrice(int itemId) const;

	// 売却
	bool SellItem(int itemId, int count = 1);

private:
	ShopSystem() : isInitialized_(false) {}
	~ShopSystem() = default;
	ShopSystem(const ShopSystem&) = delete;
	ShopSystem& operator=(const ShopSystem&) = delete;

	ShopItem* FindShopItem(int itemId);
	const ShopItem* FindShopItem(int itemId) const;

	bool isInitialized_;
	std::vector<ShopItem> items_;
};
