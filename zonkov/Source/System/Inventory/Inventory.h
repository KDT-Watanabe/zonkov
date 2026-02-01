#pragma once
#include "Data/ItemData.h"
#include <vector>
#include <memory>

// インベントリ内のアイテムスロット
struct InventorySlot
{
	int itemId;       // アイテムID（0なら空）
	int stackCount;   // スタック数
	int gridX;        // グリッド上のX位置
	int gridY;        // グリッド上のY位置
	bool isRotated;   // 回転状態（trueなら縦横入れ替え）

	InventorySlot()
		: itemId(0), stackCount(0), gridX(0), gridY(0), isRotated(false) {}

	InventorySlot(int id, int count, int x, int y, bool rotated = false)
		: itemId(id), stackCount(count), gridX(x), gridY(y), isRotated(rotated) {}

	bool IsEmpty() const { return itemId == 0; }

	// 回転を考慮したサイズ取得
	int GetWidth(const ItemData* data) const;
	int GetHeight(const ItemData* data) const;
};

// グリッド形式インベントリ
class Inventory
{
public:
	static constexpr int DEFAULT_GRID_WIDTH = 8;
	static constexpr int DEFAULT_GRID_HEIGHT = 6;
	static constexpr int CELL_SIZE = 40;

	Inventory();
	Inventory(int width, int height);  // カスタムサイズ用
	~Inventory() = default;

	// アイテム追加（自動配置）- 成功したらtrue
	bool AddItem(int itemId, int count = 1);

	// 指定位置にアイテム追加
	bool AddItemAt(int itemId, int count, int gridX, int gridY, bool rotated = false);

	// アイテム削除
	bool RemoveItem(int slotIndex, int count = 1);
	void RemoveSlot(int slotIndex);

	// スタック操作
	// 同じアイテムのスロットに追加（戻り値：追加できた数）
	int AddToStack(int slotIndex, int count);
	// 2つのスロットをマージ（同じアイテムの場合）
	bool TryMergeSlots(int fromSlotIndex, int toSlotIndex);

	// アイテム移動（回転状態も指定可能）
	bool MoveItem(int slotIndex, int newGridX, int newGridY, bool newRotated);
	bool MoveItem(int slotIndex, int newGridX, int newGridY);  // 回転維持版

	// アイテム回転（その場で回転）
	bool RotateItem(int slotIndex);

	// 指定位置に配置可能か（移動/回転プレビュー用）
	bool CanPlaceItemAt(int slotIndex, int gridX, int gridY, bool rotated) const;

	// 指定位置のスロット取得
	const InventorySlot* GetSlotAt(int gridX, int gridY) const;
	int GetSlotIndexAt(int gridX, int gridY) const;

	// 全スロット取得
	const std::vector<InventorySlot>& GetSlots() const { return slots_; }
	std::vector<InventorySlot>& GetSlots() { return slots_; }

	// スロット取得
	const InventorySlot* GetSlot(int index) const;
	InventorySlot* GetSlot(int index);

	// 総重量計算
	float GetTotalWeight() const;

	// 空きスペース確認
	bool CanFitItem(int itemId) const;
	bool CanFitItemAt(int itemId, int gridX, int gridY, bool rotated = false) const;

	// グリッド情報
	int GetGridWidth() const { return gridWidth_; }
	int GetGridHeight() const { return gridHeight_; }

	// インベントリサイズ変更（バックパック装備時に使用）
	void Resize(int newWidth, int newHeight);

	// クイックスロット
	void SetQuickSlotCount(int count);
	int GetQuickSlotCount() const { return quickSlotCount_; }
	int GetQuickSlotItem(int slot) const;
	void SetQuickSlotItem(int slot, int itemId);
	void ClearQuickSlot(int slot);
	bool UseQuickSlotItem(int slot);  // クイックスロットのアイテムを使用

	// 基本サイズ（装備なしの状態）
	static constexpr int BASE_GRID_WIDTH = 8;
	static constexpr int BASE_GRID_HEIGHT = 6;

private:
	// グリッドセルが空いているか（除外スロット指定可能）
	bool IsCellEmpty(int gridX, int gridY, int excludeSlotIndex = -1) const;

	// アイテムが置けるか（全セルチェック）
	bool CanPlaceItem(const ItemData* data, int gridX, int gridY, bool rotated, int excludeSlotIndex = -1) const;

	// 空きスペースを探す
	bool FindEmptySpace(const ItemData* data, int& outX, int& outY, bool rotated = false) const;

	// グリッドを占有/解放
	void OccupyGrid(int slotIndex, const ItemData* data, int gridX, int gridY, bool rotated);
	void ReleaseGrid(int slotIndex);

	// サイズ取得（回転考慮）
	int GetItemWidth(const ItemData* data, bool rotated) const;
	int GetItemHeight(const ItemData* data, bool rotated) const;

private:
	std::vector<InventorySlot> slots_;
	// グリッド上の各セルがどのスロットに属するか（-1なら空）
	std::vector<std::vector<int>> grid_;
	int gridWidth_;
	int gridHeight_;

	// クイックスロット（アイテムIDを格納、0なら空）
	std::vector<int> quickSlots_;
	int quickSlotCount_;
};
