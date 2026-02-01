#include "BaseScene.h"
#include "GameScene.h"
#include "System/Input/InputManager.h"
#include "System/GameData/GameData.h"
#include "System/Shop/ShopSystem.h"
#include "System/Combat/CombatSystem.h"
#include "System/Inventory/Inventory.h"
#include "Data/ItemDatabase.h"
#include <DxLib.h>

// グリッド定数（Inventoryクラスの定数と同期）
static constexpr int CELL_SIZE = Inventory::CELL_SIZE;
static constexpr int GRID_WIDTH = Inventory::DEFAULT_GRID_WIDTH;
static constexpr int GRID_HEIGHT = Inventory::DEFAULT_GRID_HEIGHT;

// タブボタン定数
static constexpr int TAB_WIDTH = 100;
static constexpr int TAB_HEIGHT = 30;
static constexpr int TAB_Y = 50;

// ショップ定数
static constexpr int SHOP_LIST_X = 50;
static constexpr int SHOP_LIST_Y = 100;
static constexpr int SHOP_ITEM_HEIGHT = 30;
static constexpr int SHOP_VISIBLE_ITEMS = 10;

BaseScene::BaseScene()
	: currentTab_(Tab::Stash)
	, nextScene_(nullptr)
	, isDragging_(false)
	, dragSlotIndex_(-1)
	, dragRotated_(false)
	, dragOffsetX_(0)
	, dragOffsetY_(0)
	, dragFromStash_(true)
	, selectedShopItem_(-1)
	, shopScrollOffset_(0)
	, selectedSellSlot_(-1)
{
}

void BaseScene::Init()
{
	ItemDatabase::GetInstance().Initialize();
	GameData::GetInstance().Initialize();
	ShopSystem::GetInstance().Initialize();
	CombatSystem::GetInstance().Initialize();
}

void BaseScene::Update()
{
	InputManager& input = InputManager::GetInstance();
	input.Update();

	// タブ切り替え
	if (input.IsMouseTriggered(0))
	{
		Vector2 mousePos = input.GetMousePosition();
		int mx = static_cast<int>(mousePos.x);
		int my = static_cast<int>(mousePos.y);

		// Stashタブ
		if (mx >= 50 && mx < 50 + TAB_WIDTH && my >= TAB_Y && my < TAB_Y + TAB_HEIGHT)
		{
			currentTab_ = Tab::Stash;
		}
		// Shopタブ
		else if (mx >= 160 && mx < 160 + TAB_WIDTH && my >= TAB_Y && my < TAB_Y + TAB_HEIGHT)
		{
			currentTab_ = Tab::Shop;
		}
		// Raidタブ
		else if (mx >= 270 && mx < 270 + TAB_WIDTH && my >= TAB_Y && my < TAB_Y + TAB_HEIGHT)
		{
			currentTab_ = Tab::Raid;
		}
	}

	// タブごとの更新
	switch (currentTab_)
	{
	case Tab::Stash:
		UpdateStashTab();
		break;
	case Tab::Shop:
		UpdateShopTab();
		break;
	case Tab::Raid:
		UpdateRaidTab();
		break;
	}
}

void BaseScene::UpdateStashTab()
{
	InputManager& input = InputManager::GetInstance();
	GameData& gameData = GameData::GetInstance();
	Inventory& stash = gameData.GetStash();

	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	// ドラッグ中の回転
	if (isDragging_ && input.IsKeyTriggered(KEY_INPUT_R))
	{
		dragRotated_ = !dragRotated_;
	}

	// ドラッグ開始
	if (!isDragging_ && input.IsMouseTriggered(0))
	{
		int gridX, gridY;
		if (ScreenToGrid(mx, my, STASH_OFFSET_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
		{
			int slotIndex = stash.GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = stash.GetSlot(slotIndex);
				if (slot)
				{
					isDragging_ = true;
					dragSlotIndex_ = slotIndex;
					dragRotated_ = slot->isRotated;
					dragFromStash_ = true;
					dragOffsetX_ = (gridX - slot->gridX) * CELL_SIZE;
					dragOffsetY_ = (gridY - slot->gridY) * CELL_SIZE;
				}
			}
		}
	}

	// ドラッグ終了
	if (isDragging_ && input.IsMouseReleased(0))
	{
		int gridX, gridY;
		if (ScreenToGrid(mx - dragOffsetX_, my - dragOffsetY_, STASH_OFFSET_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
		{
			stash.MoveItem(dragSlotIndex_, gridX, gridY, dragRotated_);
		}
		isDragging_ = false;
		dragSlotIndex_ = -1;
	}
}

void BaseScene::UpdateShopTab()
{
	InputManager& input = InputManager::GetInstance();
	ShopSystem& shop = ShopSystem::GetInstance();
	GameData& gameData = GameData::GetInstance();
	Inventory& stash = gameData.GetStash();
	const auto& items = shop.GetShopItems();

	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	// ショップリストのスクロール
	int wheel = input.GetMouseWheelRotation();
	if (wheel != 0)
	{
		shopScrollOffset_ -= wheel;
		int maxScroll = static_cast<int>(items.size()) - SHOP_VISIBLE_ITEMS;
		if (maxScroll < 0) maxScroll = 0;
		if (shopScrollOffset_ < 0) shopScrollOffset_ = 0;
		if (shopScrollOffset_ > maxScroll) shopScrollOffset_ = maxScroll;
	}

	// クリック処理
	if (input.IsMouseTriggered(0))
	{
		// ショップアイテム選択（購入用）
		if (mx >= SHOP_LIST_X && mx < SHOP_LIST_X + 300 && my >= SHOP_LIST_Y)
		{
			int itemIndex = (my - SHOP_LIST_Y) / SHOP_ITEM_HEIGHT + shopScrollOffset_;
			if (itemIndex >= 0 && itemIndex < static_cast<int>(items.size()))
			{
				selectedShopItem_ = itemIndex;
				selectedSellSlot_ = -1;  // 購入選択したら売却選択解除
			}
		}

		// スタッシュアイテム選択（売却用）
		int gridX, gridY;
		static constexpr int SHOP_STASH_X = 500;
		if (ScreenToGrid(mx, my, SHOP_STASH_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
		{
			int slotIndex = stash.GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				selectedSellSlot_ = slotIndex;
				selectedShopItem_ = -1;  // 売却選択したら購入選択解除
			}
		}

		// 購入ボタン
		if (selectedShopItem_ >= 0 && mx >= 400 && mx < 500 && my >= 365 && my < 395)
		{
			const ShopItem& item = items[selectedShopItem_];
			shop.BuyItem(item.itemId, 1);
		}

		// 売却ボタン
		if (selectedSellSlot_ >= 0 && mx >= 400 && mx < 500 && my >= 455 && my < 485)
		{
			const InventorySlot* slot = stash.GetSlot(selectedSellSlot_);
			if (slot && !slot->IsEmpty())
			{
				int sellPrice = shop.GetSellPrice(slot->itemId);
				if (sellPrice > 0)
				{
					// 1個売却
					gameData.AddMoney(sellPrice);
					stash.RemoveItem(selectedSellSlot_, 1);

					// スロットが空になったら選択解除
					const InventorySlot* updated = stash.GetSlot(selectedSellSlot_);
					if (!updated || updated->IsEmpty())
					{
						selectedSellSlot_ = -1;
					}
				}
			}
		}
	}
}

void BaseScene::UpdateRaidTab()
{
	InputManager& input = InputManager::GetInstance();
	GameData& gameData = GameData::GetInstance();
	Inventory& stash = gameData.GetStash();
	Inventory& raidInv = gameData.GetRaidInventory();

	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	// ドラッグ中の回転
	if (isDragging_ && input.IsKeyTriggered(KEY_INPUT_R))
	{
		dragRotated_ = !dragRotated_;
	}

	// ドラッグ開始（スタッシュから）
	if (!isDragging_ && input.IsMouseTriggered(0))
	{
		int gridX, gridY;
		// スタッシュからドラッグ
		if (ScreenToGrid(mx, my, STASH_OFFSET_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
		{
			int slotIndex = stash.GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = stash.GetSlot(slotIndex);
				if (slot)
				{
					isDragging_ = true;
					dragSlotIndex_ = slotIndex;
					dragRotated_ = slot->isRotated;
					dragFromStash_ = true;
					dragOffsetX_ = (gridX - slot->gridX) * CELL_SIZE;
					dragOffsetY_ = (gridY - slot->gridY) * CELL_SIZE;
				}
			}
		}
		// レイドインベントリからドラッグ
		else if (ScreenToGrid(mx, my, RAID_INV_OFFSET_X, RAID_INV_OFFSET_Y, raidInv.GetGridWidth(), raidInv.GetGridHeight(), gridX, gridY))
		{
			int slotIndex = raidInv.GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = raidInv.GetSlot(slotIndex);
				if (slot)
				{
					isDragging_ = true;
					dragSlotIndex_ = slotIndex;
					dragRotated_ = slot->isRotated;
					dragFromStash_ = false;
					dragOffsetX_ = (gridX - slot->gridX) * CELL_SIZE;
					dragOffsetY_ = (gridY - slot->gridY) * CELL_SIZE;
				}
			}
		}
		// 装備スロットクリックで解除
		else
		{
			int equipSlotIndex = GetEquipSlotAtPosition(mx, my);
			if (equipSlotIndex >= 0)
			{
				TryUnequipItem(static_cast<EquipSlot>(equipSlotIndex));
			}
		}
	}

	// ドラッグ終了
	if (isDragging_ && input.IsMouseReleased(0))
	{
		int gridX, gridY;
		Inventory& srcInv = dragFromStash_ ? stash : raidInv;
		const InventorySlot* slot = srcInv.GetSlot(dragSlotIndex_);
		bool placed = false;

		if (slot)
		{
			// 装備スロットにドロップ
			int equipSlotIndex = GetEquipSlotAtPosition(mx, my);
			if (equipSlotIndex >= 0)
			{
				if (dragFromStash_)
				{
					placed = TryEquipFromStash(dragSlotIndex_);
				}
				else
				{
					placed = TryEquipFromRaid(dragSlotIndex_);
				}
			}

			if (!placed)
			{
				// スタッシュにドロップ
				if (ScreenToGrid(mx - dragOffsetX_, my - dragOffsetY_, STASH_OFFSET_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
				{
					if (dragFromStash_)
					{
						stash.MoveItem(dragSlotIndex_, gridX, gridY, dragRotated_);
					}
					else
					{
						// レイドインベントリからスタッシュへ移動
						if (stash.CanFitItemAt(slot->itemId, gridX, gridY, dragRotated_))
						{
							stash.AddItemAt(slot->itemId, slot->stackCount, gridX, gridY, dragRotated_);
							raidInv.RemoveItem(dragSlotIndex_, slot->stackCount);
						}
					}
				}
				// レイドインベントリにドロップ
				else if (ScreenToGrid(mx - dragOffsetX_, my - dragOffsetY_, RAID_INV_OFFSET_X, RAID_INV_OFFSET_Y, raidInv.GetGridWidth(), raidInv.GetGridHeight(), gridX, gridY))
				{
					if (!dragFromStash_)
					{
						raidInv.MoveItem(dragSlotIndex_, gridX, gridY, dragRotated_);
					}
					else
					{
						// スタッシュからレイドインベントリへ移動
						if (raidInv.CanFitItemAt(slot->itemId, gridX, gridY, dragRotated_))
						{
							raidInv.AddItemAt(slot->itemId, slot->stackCount, gridX, gridY, dragRotated_);
							stash.RemoveItem(dragSlotIndex_, slot->stackCount);
						}
					}
				}
			}
		}

		isDragging_ = false;
		dragSlotIndex_ = -1;
	}

	// 出撃ボタン
	if (input.IsMouseTriggered(0))
	{
		if (mx >= 1100 && mx < 1200 && my >= 600 && my < 640)
		{
			gameData.PrepareForRaid();
			nextScene_ = std::make_unique<GameScene>();
		}
	}
}

void BaseScene::Draw()
{
	// 背景
	DrawBox(0, 0, 1280, 720, GetColor(40, 40, 50), TRUE);

	// タブ
	DrawTabs();

	// 所持金
	DrawMoney();

	// タブごとの描画
	switch (currentTab_)
	{
	case Tab::Stash:
		DrawStashTab();
		break;
	case Tab::Shop:
		DrawShopTab();
		break;
	case Tab::Raid:
		DrawRaidTab();
		break;
	}

	// ツールチップ（最前面に描画）
	DrawItemTooltip();
}

void BaseScene::DrawTabs() const
{
	unsigned int activeColor = GetColor(100, 100, 150);
	unsigned int inactiveColor = GetColor(60, 60, 80);
	unsigned int textColor = GetColor(255, 255, 255);

	// Stashタブ
	DrawBox(50, TAB_Y, 50 + TAB_WIDTH, TAB_Y + TAB_HEIGHT,
		currentTab_ == Tab::Stash ? activeColor : inactiveColor, TRUE);
	DrawString(70, TAB_Y + 8, "倉庫", textColor);

	// Shopタブ
	DrawBox(160, TAB_Y, 160 + TAB_WIDTH, TAB_Y + TAB_HEIGHT,
		currentTab_ == Tab::Shop ? activeColor : inactiveColor, TRUE);
	DrawString(175, TAB_Y + 8, "ショップ", textColor);

	// Raidタブ
	DrawBox(270, TAB_Y, 270 + TAB_WIDTH, TAB_Y + TAB_HEIGHT,
		currentTab_ == Tab::Raid ? activeColor : inactiveColor, TRUE);
	DrawString(285, TAB_Y + 8, "レイド", textColor);
}

void BaseScene::DrawMoney() const
{
	int money = GameData::GetInstance().GetMoney();
	DrawFormatString(1100, 20, GetColor(255, 215, 0), "所持金: %d", money);
}

void BaseScene::DrawStashTab() const
{
	const Inventory& stash = GameData::GetInstance().GetStash();

	DrawString(STASH_OFFSET_X, STASH_OFFSET_Y - 20, "倉庫", GetColor(255, 255, 255));
	DrawInventoryGrid(stash, STASH_OFFSET_X, STASH_OFFSET_Y, true);

	// ドラッグ中のアイテム
	if (isDragging_ && dragFromStash_)
	{
		InputManager& input = InputManager::GetInstance();
		const InventorySlot* slot = stash.GetSlot(dragSlotIndex_);
		if (slot)
		{
			const ItemData* itemData = ItemDatabase::GetInstance().GetItem(slot->itemId);
			if (itemData)
			{
				Vector2 mousePos = input.GetMousePosition();
				int w = dragRotated_ ? itemData->gridHeight : itemData->gridWidth;
				int h = dragRotated_ ? itemData->gridWidth : itemData->gridHeight;
				int drawX = static_cast<int>(mousePos.x) - dragOffsetX_;
				int drawY = static_cast<int>(mousePos.y) - dragOffsetY_;

				DrawBox(drawX, drawY, drawX + w * CELL_SIZE, drawY + h * CELL_SIZE,
					GetColor(100, 150, 200), TRUE);
				DrawBox(drawX, drawY, drawX + w * CELL_SIZE, drawY + h * CELL_SIZE,
					GetColor(200, 200, 255), FALSE);
			}
		}
	}
}

void BaseScene::DrawShopTab() const
{
	ShopSystem& shop = ShopSystem::GetInstance();
	const auto& items = shop.GetShopItems();
	const ItemDatabase& itemDb = ItemDatabase::GetInstance();

	DrawString(SHOP_LIST_X, SHOP_LIST_Y - 20, "商品一覧", GetColor(255, 255, 255));

	// 商品リスト
	int visibleCount = 0;
	for (int i = shopScrollOffset_; i < static_cast<int>(items.size()) && visibleCount < SHOP_VISIBLE_ITEMS; ++i, ++visibleCount)
	{
		const ShopItem& shopItem = items[i];
		const ItemData* itemData = itemDb.GetItem(shopItem.itemId);
		if (!itemData) continue;

		int y = SHOP_LIST_Y + visibleCount * SHOP_ITEM_HEIGHT;
		unsigned int bgColor = (i == selectedShopItem_) ? GetColor(80, 80, 120) : GetColor(50, 50, 70);

		DrawBox(SHOP_LIST_X, y, SHOP_LIST_X + 300, y + SHOP_ITEM_HEIGHT - 2, bgColor, TRUE);

		// アイテム名
		DrawString(SHOP_LIST_X + 5, y + 7, itemData->name.c_str(), GetColor(255, 255, 255));

		// 価格
		if (shopItem.buyPrice > 0)
		{
			DrawFormatString(SHOP_LIST_X + 200, y + 7, GetColor(255, 215, 0), "%d", shopItem.buyPrice);
		}
		else
		{
			DrawString(SHOP_LIST_X + 200, y + 7, "---", GetColor(128, 128, 128));
		}

		// 在庫
		if (shopItem.stock == -1)
		{
			DrawString(SHOP_LIST_X + 260, y + 7, "inf", GetColor(200, 200, 200));
		}
		else if (shopItem.stock > 0)
		{
			DrawFormatString(SHOP_LIST_X + 260, y + 7, GetColor(200, 200, 200), "x%d", shopItem.stock);
		}
		else
		{
			DrawString(SHOP_LIST_X + 260, y + 7, "x0", GetColor(128, 128, 128));
		}
	}

	// スタッシュ（売却用）
	static constexpr int SHOP_STASH_X = 500;
	DrawString(SHOP_STASH_X, STASH_OFFSET_Y - 20, "倉庫 クリックで売却", GetColor(255, 255, 255));
	const Inventory& stash = GameData::GetInstance().GetStash();
	DrawInventoryGrid(stash, SHOP_STASH_X, STASH_OFFSET_Y, true);

	// 選択中のスタッシュアイテムをハイライト
	if (selectedSellSlot_ >= 0)
	{
		const InventorySlot* slot = stash.GetSlot(selectedSellSlot_);
		if (slot && !slot->IsEmpty())
		{
			const ItemData* itemData = itemDb.GetItem(slot->itemId);
			if (itemData)
			{
				int w = slot->GetWidth(itemData);
				int h = slot->GetHeight(itemData);
				int x = SHOP_STASH_X + slot->gridX * CELL_SIZE;
				int y = STASH_OFFSET_Y + slot->gridY * CELL_SIZE;
				DrawBox(x, y, x + w * CELL_SIZE, y + h * CELL_SIZE, GetColor(255, 200, 100), FALSE);
				DrawBox(x + 1, y + 1, x + w * CELL_SIZE - 1, y + h * CELL_SIZE - 1, GetColor(255, 200, 100), FALSE);
			}
		}
	}

	// 購入ボタン
	if (selectedShopItem_ >= 0 && selectedShopItem_ < static_cast<int>(items.size()))
	{
		const ShopItem& selected = items[selectedShopItem_];
		const ItemData* selectedData = itemDb.GetItem(selected.itemId);

		DrawString(400, 330, "選択中:", GetColor(200, 200, 200));
		if (selectedData)
		{
			DrawString(400, 345, selectedData->name.c_str(), GetColor(255, 255, 255));
		}

		if (selected.buyPrice > 0 && selected.stock != 0)
		{
			DrawBox(400, 365, 500, 395, GetColor(50, 120, 50), TRUE);
			DrawBox(400, 365, 500, 395, GetColor(100, 200, 100), FALSE);
			DrawFormatString(410, 372, GetColor(255, 255, 255), "購入 (%d)", selected.buyPrice);
		}
	}

	// 売却ボタン
	if (selectedSellSlot_ >= 0)
	{
		const InventorySlot* slot = stash.GetSlot(selectedSellSlot_);
		if (slot && !slot->IsEmpty())
		{
			const ItemData* itemData = itemDb.GetItem(slot->itemId);
			int sellPrice = shop.GetSellPrice(slot->itemId);

			DrawString(400, 420, "売却:", GetColor(200, 200, 200));
			if (itemData)
			{
				DrawFormatString(400, 435, GetColor(255, 255, 255), "%s x%d", itemData->name.c_str(), slot->stackCount);
			}

			if (sellPrice > 0)
			{
				DrawBox(400, 455, 500, 485, GetColor(120, 80, 50), TRUE);
				DrawBox(400, 455, 500, 485, GetColor(200, 150, 100), FALSE);
				DrawFormatString(410, 462, GetColor(255, 255, 255), "売却 (%d)", sellPrice);
			}
			else
			{
				DrawString(400, 455, "売却不可", GetColor(128, 128, 128));
			}
		}
	}
}

void BaseScene::DrawRaidTab() const
{
	const GameData& gameData = GameData::GetInstance();
	const Inventory& stash = gameData.GetStash();
	const Inventory& raidInv = gameData.GetRaidInventory();

	// スタッシュ
	DrawString(STASH_OFFSET_X, STASH_OFFSET_Y - 20, "倉庫", GetColor(255, 255, 255));
	DrawInventoryGrid(stash, STASH_OFFSET_X, STASH_OFFSET_Y, true);

	// レイドインベントリ
	DrawString(RAID_INV_OFFSET_X, RAID_INV_OFFSET_Y - 20, "レイド装備", GetColor(255, 255, 255));
	DrawInventoryGrid(raidInv, RAID_INV_OFFSET_X, RAID_INV_OFFSET_Y, false);

	// 装備UI
	DrawEquipmentUI();

	// 出撃ボタン
	DrawBox(1100, 600, 1200, 640, GetColor(150, 50, 50), TRUE);
	DrawString(1120, 612, "出撃", GetColor(255, 255, 255));

	// ドラッグ中のアイテム
	if (isDragging_)
	{
		InputManager& input = InputManager::GetInstance();
		const Inventory& srcInv = dragFromStash_ ? stash : raidInv;
		const InventorySlot* slot = srcInv.GetSlot(dragSlotIndex_);
		if (slot)
		{
			const ItemData* itemData = ItemDatabase::GetInstance().GetItem(slot->itemId);
			if (itemData)
			{
				Vector2 mousePos = input.GetMousePosition();
				int w = dragRotated_ ? itemData->gridHeight : itemData->gridWidth;
				int h = dragRotated_ ? itemData->gridWidth : itemData->gridHeight;
				int drawX = static_cast<int>(mousePos.x) - dragOffsetX_;
				int drawY = static_cast<int>(mousePos.y) - dragOffsetY_;

				DrawBox(drawX, drawY, drawX + w * CELL_SIZE, drawY + h * CELL_SIZE,
					GetColor(100, 150, 200), TRUE);
				DrawBox(drawX, drawY, drawX + w * CELL_SIZE, drawY + h * CELL_SIZE,
					GetColor(200, 200, 255), FALSE);
			}
		}
	}
}

void BaseScene::DrawInventoryGrid(const Inventory& inv, int offsetX, int offsetY, bool isStash) const
{
	unsigned int gridColor = GetColor(80, 80, 100);
	unsigned int cellColor = GetColor(50, 50, 60);

	// インベントリ実際のサイズを使用
	int gridWidth = inv.GetGridWidth();
	int gridHeight = inv.GetGridHeight();

	// グリッド背景
	DrawBox(offsetX, offsetY, offsetX + gridWidth * CELL_SIZE, offsetY + gridHeight * CELL_SIZE,
		cellColor, TRUE);

	// グリッド線
	for (int x = 0; x <= gridWidth; ++x)
	{
		DrawLine(offsetX + x * CELL_SIZE, offsetY,
			offsetX + x * CELL_SIZE, offsetY + gridHeight * CELL_SIZE, gridColor);
	}
	for (int y = 0; y <= gridHeight; ++y)
	{
		DrawLine(offsetX, offsetY + y * CELL_SIZE,
			offsetX + gridWidth * CELL_SIZE, offsetY + y * CELL_SIZE, gridColor);
	}

	// アイテム描画
	const ItemDatabase& itemDb = ItemDatabase::GetInstance();
	for (const auto& slot : inv.GetSlots())
	{
		if (slot.IsEmpty()) continue;

		// ドラッグ中のアイテムはスキップ
		if (isDragging_ && &slot == inv.GetSlot(dragSlotIndex_) &&
			((isStash && dragFromStash_) || (!isStash && !dragFromStash_)))
		{
			continue;
		}

		const ItemData* itemData = itemDb.GetItem(slot.itemId);
		if (!itemData) continue;

		int w = slot.GetWidth(itemData);
		int h = slot.GetHeight(itemData);
		int x = offsetX + slot.gridX * CELL_SIZE;
		int y = offsetY + slot.gridY * CELL_SIZE;

		DrawBox(x + 2, y + 2, x + w * CELL_SIZE - 2, y + h * CELL_SIZE - 2,
			GetColor(70, 100, 130), TRUE);
		DrawBox(x + 2, y + 2, x + w * CELL_SIZE - 2, y + h * CELL_SIZE - 2,
			GetColor(120, 150, 180), FALSE);

		// アイテム名（短縮）
		std::string shortName = itemData->name.substr(0, 8);
		DrawString(x + 4, y + 4, shortName.c_str(), GetColor(255, 255, 255));

		// スタック数
		if (slot.stackCount > 1)
		{
			DrawFormatString(x + 4, y + h * CELL_SIZE - 16, GetColor(255, 255, 200), "x%d", slot.stackCount);
		}
	}
}

bool BaseScene::ScreenToGrid(int screenX, int screenY, int offsetX, int offsetY, int gridWidth, int gridHeight, int& gridX, int& gridY) const
{
	int localX = screenX - offsetX;
	int localY = screenY - offsetY;

	if (localX < 0 || localY < 0) return false;

	gridX = localX / CELL_SIZE;
	gridY = localY / CELL_SIZE;

	if (gridX >= gridWidth || gridY >= gridHeight) return false;

	return true;
}

std::unique_ptr<IScene> BaseScene::GetNextScene()
{
	return std::move(nextScene_);
}

void BaseScene::DrawEquipmentUI() const
{
	const char* slotNames[] = { "Weapon", "Shield", "Armor", "Backpck", "Rig" };
	const GameData& gameData = GameData::GetInstance();
	const ItemDatabase& itemDb = ItemDatabase::GetInstance();

	int slotCount = static_cast<int>(EquipSlot::Count);

	// 背景
	int bgX = EQUIP_OFFSET_X - 5;
	int bgY = EQUIP_OFFSET_Y - 25;
	int bgW = (EQUIP_SLOT_SIZE + EQUIP_SLOT_GAP) * slotCount + 5;
	int bgH = EQUIP_SLOT_SIZE + 45;
	DrawBox(bgX, bgY, bgX + bgW, bgY + bgH, GetColor(30, 30, 40), TRUE);
	DrawBox(bgX, bgY, bgX + bgW, bgY + bgH, GetColor(100, 100, 120), FALSE);
	DrawString(bgX + 5, bgY + 5, "装備", GetColor(255, 255, 255));

	for (int i = 0; i < slotCount; ++i)
	{
		int x = EQUIP_OFFSET_X + i * (EQUIP_SLOT_SIZE + EQUIP_SLOT_GAP);
		int y = EQUIP_OFFSET_Y;

		// スロット背景
		unsigned int slotColor = GetColor(50, 50, 60);
		int itemId = gameData.GetEquippedItem(static_cast<EquipSlot>(i));
		if (itemId != 0)
		{
			const ItemData* data = itemDb.GetItem(itemId);
			if (data)
			{
				if (data->type == ItemType::Weapon)
					slotColor = GetColor(100, 60, 60);
				else if (data->type == ItemType::Armor)
					slotColor = GetColor(60, 60, 100);
				else if (data->type == ItemType::Backpack)
					slotColor = GetColor(90, 70, 50);
				else if (data->type == ItemType::Rig)
					slotColor = GetColor(50, 90, 50);
			}
		}
		DrawBox(x, y, x + EQUIP_SLOT_SIZE, y + EQUIP_SLOT_SIZE, slotColor, TRUE);
		DrawBox(x, y, x + EQUIP_SLOT_SIZE, y + EQUIP_SLOT_SIZE, GetColor(80, 80, 90), FALSE);

		// スロット名
		DrawString(x + 2, y + EQUIP_SLOT_SIZE + 2, slotNames[i], GetColor(150, 150, 150));

		// 装備中アイテム
		if (itemId != 0)
		{
			const ItemData* data = itemDb.GetItem(itemId);
			if (data)
			{
				std::string shortName = data->name.substr(0, 7);
				DrawString(x + 3, y + 3, shortName.c_str(), GetColor(255, 255, 255));
			}
		}
	}
}

int BaseScene::GetEquipSlotAtPosition(int screenX, int screenY) const
{
	for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i)
	{
		int x = EQUIP_OFFSET_X + i * (EQUIP_SLOT_SIZE + EQUIP_SLOT_GAP);
		int y = EQUIP_OFFSET_Y;

		if (screenX >= x && screenX < x + EQUIP_SLOT_SIZE &&
			screenY >= y && screenY < y + EQUIP_SLOT_SIZE)
		{
			return i;
		}
	}
	return -1;
}

bool BaseScene::TryEquipFromStash(int slotIndex)
{
	GameData& gameData = GameData::GetInstance();
	Inventory& stash = gameData.GetStash();
	const InventorySlot* slot = stash.GetSlot(slotIndex);
	if (!slot || slot->IsEmpty()) return false;

	EquipSlot equipSlot = CombatSystem::GetInstance().GetEquipSlotForItem(slot->itemId);
	if (equipSlot == EquipSlot::Count) return false;

	// 既に装備がある場合は入れ替え（スタッシュに戻す）
	int currentEquipped = gameData.GetEquippedItem(equipSlot);
	if (currentEquipped != 0)
	{
		if (!stash.AddItem(currentEquipped, 1))
		{
			return false;
		}
	}

	gameData.SetEquippedItem(equipSlot, slot->itemId);
	stash.RemoveItem(slotIndex, 1);
	return true;
}

bool BaseScene::TryEquipFromRaid(int slotIndex)
{
	GameData& gameData = GameData::GetInstance();
	Inventory& raidInv = gameData.GetRaidInventory();
	const InventorySlot* slot = raidInv.GetSlot(slotIndex);
	if (!slot || slot->IsEmpty()) return false;

	EquipSlot equipSlot = CombatSystem::GetInstance().GetEquipSlotForItem(slot->itemId);
	if (equipSlot == EquipSlot::Count) return false;

	// 既に装備がある場合は入れ替え（レイドインベントリに戻す）
	int currentEquipped = gameData.GetEquippedItem(equipSlot);
	if (currentEquipped != 0)
	{
		if (!raidInv.AddItem(currentEquipped, 1))
		{
			return false;
		}
	}

	gameData.SetEquippedItem(equipSlot, slot->itemId);
	raidInv.RemoveItem(slotIndex, 1);
	return true;
}

bool BaseScene::TryUnequipItem(EquipSlot slot)
{
	GameData& gameData = GameData::GetInstance();
	int itemId = gameData.GetEquippedItem(slot);
	if (itemId == 0) return false;

	// スタッシュに戻す
	Inventory& stash = gameData.GetStash();
	if (!stash.AddItem(itemId, 1))
	{
		return false;
	}

	gameData.UnequipItem(slot);
	return true;
}

void BaseScene::DrawItemTooltip() const
{
	if (isDragging_) return;

	InputManager& input = InputManager::GetInstance();
	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	int itemId = 0;
	const GameData& gameData = GameData::GetInstance();
	const ItemDatabase& itemDb = ItemDatabase::GetInstance();

	// タブごとにアイテムを検出
	if (currentTab_ == Tab::Stash || currentTab_ == Tab::Raid)
	{
		// スタッシュ
		int gridX, gridY;
		const Inventory& stash = gameData.GetStash();
		if (ScreenToGrid(mx, my, STASH_OFFSET_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
		{
			int slotIndex = stash.GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = stash.GetSlot(slotIndex);
				if (slot && !slot->IsEmpty())
				{
					itemId = slot->itemId;
				}
			}
		}

		// レイドインベントリ（Raidタブのみ）
		if (itemId == 0 && currentTab_ == Tab::Raid)
		{
			const Inventory& raidInv = gameData.GetRaidInventory();
			if (ScreenToGrid(mx, my, RAID_INV_OFFSET_X, RAID_INV_OFFSET_Y, raidInv.GetGridWidth(), raidInv.GetGridHeight(), gridX, gridY))
			{
				int slotIndex = raidInv.GetSlotIndexAt(gridX, gridY);
				if (slotIndex >= 0)
				{
					const InventorySlot* slot = raidInv.GetSlot(slotIndex);
					if (slot && !slot->IsEmpty())
					{
						itemId = slot->itemId;
					}
				}
			}
		}

		// 装備スロット（Raidタブのみ）
		if (itemId == 0 && currentTab_ == Tab::Raid)
		{
			int equipSlotIndex = GetEquipSlotAtPosition(mx, my);
			if (equipSlotIndex >= 0)
			{
				itemId = gameData.GetEquippedItem(static_cast<EquipSlot>(equipSlotIndex));
			}
		}
	}
	else if (currentTab_ == Tab::Shop)
	{
		// ショップのスタッシュ
		static constexpr int SHOP_STASH_X = 500;
		int gridX, gridY;
		const Inventory& stash = gameData.GetStash();
		if (ScreenToGrid(mx, my, SHOP_STASH_X, STASH_OFFSET_Y, stash.GetGridWidth(), stash.GetGridHeight(), gridX, gridY))
		{
			int slotIndex = stash.GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = stash.GetSlot(slotIndex);
				if (slot && !slot->IsEmpty())
				{
					itemId = slot->itemId;
				}
			}
		}
	}

	if (itemId == 0) return;

	// アイテム情報取得
	const ItemData* item = itemDb.GetItem(itemId);
	if (!item) return;

	// ツールチップ描画
	int tooltipX = mx + 15;
	int tooltipY = my + 15;
	int tooltipW = 200;
	int tooltipH = 80;

	const WeaponData* weapon = CombatSystem::GetInstance().GetWeaponData(itemId);
	const ArmorData* armor = CombatSystem::GetInstance().GetArmorData(itemId);
	if (weapon || armor) tooltipH = 120;

	if (tooltipX + tooltipW > 1280) tooltipX = mx - tooltipW - 5;
	if (tooltipY + tooltipH > 720) tooltipY = my - tooltipH - 5;

	DrawBox(tooltipX, tooltipY, tooltipX + tooltipW, tooltipY + tooltipH, GetColor(20, 20, 30), TRUE);
	DrawBox(tooltipX, tooltipY, tooltipX + tooltipW, tooltipY + tooltipH, GetColor(100, 100, 120), FALSE);

	int textY = tooltipY + 5;

	unsigned int nameColor = GetColor(255, 255, 255);
	if (item->type == ItemType::Weapon) nameColor = GetColor(255, 150, 150);
	else if (item->type == ItemType::Armor) nameColor = GetColor(150, 150, 255);
	else if (item->type == ItemType::Valuable) nameColor = GetColor(255, 215, 0);
	DrawString(tooltipX + 5, textY, item->name.c_str(), nameColor);
	textY += 18;

	if (!item->description.empty())
	{
		DrawString(tooltipX + 5, textY, item->description.c_str(), GetColor(180, 180, 180));
		textY += 16;
	}

	DrawFormatString(tooltipX + 5, textY, GetColor(150, 150, 150), "Weight: %.2f kg", item->weight);
	textY += 16;

	if (weapon)
	{
		const char* atkTypeName = "Unknown";
		switch (weapon->attackType)
		{
		case AttackType::Slash: atkTypeName = "Slash"; break;
		case AttackType::Pierce: atkTypeName = "Pierce"; break;
		case AttackType::Blunt: atkTypeName = "Blunt"; break;
		default: break;
		}
		DrawFormatString(tooltipX + 5, textY, GetColor(255, 200, 100), "DMG: %d  Type: %s", weapon->baseDamage, atkTypeName);
		textY += 16;
		DrawFormatString(tooltipX + 5, textY, GetColor(200, 200, 150), "Range: %.0f  Speed: %.1f", weapon->range, weapon->attackSpeed);
	}

	if (armor)
	{
		DrawFormatString(tooltipX + 5, textY, GetColor(100, 200, 255), "DEF: %d", armor->baseDefense);
		textY += 16;
		DrawFormatString(tooltipX + 5, textY, GetColor(150, 200, 150), "Slash:%.0f%% Pierce:%.0f%% Blunt:%.0f%%",
			armor->resistances.slash * 100, armor->resistances.pierce * 100, armor->resistances.blunt * 100);
		if (armor->isShield && armor->blockChance > 0)
		{
			textY += 16;
			DrawFormatString(tooltipX + 5, textY, GetColor(200, 200, 100), "Block: %.0f%%", armor->blockChance * 100);
		}
	}
}
