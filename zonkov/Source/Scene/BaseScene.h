#pragma once
#include "IScene.h"
#include "Utility/Vector2.h"
#include "Data/CombatTypes.h"
#include <memory>

class BaseScene : public IScene
{
public:
	enum class Tab
	{
		Stash,
		Shop,
		Raid
	};

	BaseScene();
	~BaseScene() override = default;

	void Init() override;
	void Update() override;
	void Draw() override;
	std::unique_ptr<IScene> GetNextScene() override;

private:
	void UpdateStashTab();
	void UpdateShopTab();
	void UpdateRaidTab();

	void DrawStashTab() const;
	void DrawShopTab() const;
	void DrawRaidTab() const;
	void DrawTabs() const;
	void DrawMoney() const;
	void DrawEquipmentUI() const;
	void DrawItemTooltip() const;

	// インベントリ操作
	void DrawInventoryGrid(const class Inventory& inv, int offsetX, int offsetY, bool isStash) const;
	bool ScreenToGrid(int screenX, int screenY, int offsetX, int offsetY, int gridWidth, int gridHeight, int& gridX, int& gridY) const;

	// 装備操作
	int GetEquipSlotAtPosition(int screenX, int screenY) const;
	bool TryEquipFromStash(int slotIndex);
	bool TryEquipFromRaid(int slotIndex);
	bool TryUnequipItem(EquipSlot slot);

private:
	Tab currentTab_;
	std::unique_ptr<IScene> nextScene_;

	// スタッシュ操作
	bool isDragging_;
	int dragSlotIndex_;
	bool dragRotated_;
	int dragOffsetX_;
	int dragOffsetY_;
	bool dragFromStash_;  // ドラッグ元がスタッシュか

	// ショップ
	int selectedShopItem_;
	int shopScrollOffset_;
	int selectedSellSlot_;  // 売却対象のスタッシュスロット

	// UI定数
	static constexpr int STASH_OFFSET_X = 50;
	static constexpr int STASH_OFFSET_Y = 100;
	static constexpr int RAID_INV_OFFSET_X = 500;
	static constexpr int RAID_INV_OFFSET_Y = 100;

	// 装備UI定数
	static constexpr int EQUIP_OFFSET_X = 500;
	static constexpr int EQUIP_OFFSET_Y = 450;
	static constexpr int EQUIP_SLOT_SIZE = 50;
	static constexpr int EQUIP_SLOT_GAP = 8;
};
