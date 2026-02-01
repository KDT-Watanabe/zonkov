#pragma once
#include "ItemData.h"
#include <vector>

// アイテムデータベース（シングルトン）
class ItemDatabase
{
public:
	static ItemDatabase& GetInstance()
	{
		static ItemDatabase instance;
		return instance;
	}

	// 初期化（アイテムデータ登録）
	void Initialize();

	// アイテムデータ取得
	const ItemData* GetItemData(int itemId) const;
	const ItemData* GetItem(int itemId) const { return GetItemData(itemId); }

	// 全アイテム数
	int GetItemCount() const { return static_cast<int>(items_.size()); }

private:
	ItemDatabase() : isInitialized_(false) {}
	~ItemDatabase() = default;
	ItemDatabase(const ItemDatabase&) = delete;
	ItemDatabase& operator=(const ItemDatabase&) = delete;

	void RegisterItem(const ItemData& data);

	bool isInitialized_;
	std::vector<ItemData> items_;
};
