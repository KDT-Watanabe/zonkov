#include "Inventory.h"
#include "Data/ItemDatabase.h"

// InventorySlot メンバ関数
int InventorySlot::GetWidth(const ItemData* data) const
{
	if (!data) return 1;
	return isRotated ? data->gridHeight : data->gridWidth;
}

int InventorySlot::GetHeight(const ItemData* data) const
{
	if (!data) return 1;
	return isRotated ? data->gridWidth : data->gridHeight;
}

// Inventory
Inventory::Inventory()
	: gridWidth_(DEFAULT_GRID_WIDTH)
	, gridHeight_(DEFAULT_GRID_HEIGHT)
	, quickSlotCount_(0)
{
	grid_.resize(gridHeight_, std::vector<int>(gridWidth_, -1));
}

Inventory::Inventory(int width, int height)
	: gridWidth_(width)
	, gridHeight_(height)
	, quickSlotCount_(0)
{
	grid_.resize(gridHeight_, std::vector<int>(gridWidth_, -1));
}

bool Inventory::AddItem(int itemId, int count)
{
	const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
	if (!data) return false;

	// まず既存スロットにスタックできるか確認
	if (data->maxStack > 1)
	{
		for (auto& slot : slots_)
		{
			if (slot.itemId == itemId && slot.stackCount < data->maxStack)
			{
				int canAdd = data->maxStack - slot.stackCount;
				int toAdd = (count < canAdd) ? count : canAdd;
				slot.stackCount += toAdd;
				count -= toAdd;
				if (count <= 0) return true;
			}
		}
	}

	// 新しいスロットに配置
	while (count > 0)
	{
		int x, y;
		// 通常向きで探す
		if (!FindEmptySpace(data, x, y, false))
		{
			// 回転して探す
			if (!FindEmptySpace(data, x, y, true))
			{
				return false;  // 空きスペースなし
			}
			int toAdd = (count < data->maxStack) ? count : data->maxStack;
			int slotIndex = static_cast<int>(slots_.size());
			slots_.emplace_back(itemId, toAdd, x, y, true);
			OccupyGrid(slotIndex, data, x, y, true);
			count -= toAdd;
		}
		else
		{
			int toAdd = (count < data->maxStack) ? count : data->maxStack;
			int slotIndex = static_cast<int>(slots_.size());
			slots_.emplace_back(itemId, toAdd, x, y, false);
			OccupyGrid(slotIndex, data, x, y, false);
			count -= toAdd;
		}
	}

	return true;
}

bool Inventory::AddItemAt(int itemId, int count, int gridX, int gridY, bool rotated)
{
	const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
	if (!data) return false;

	if (!CanPlaceItem(data, gridX, gridY, rotated)) return false;

	int slotIndex = static_cast<int>(slots_.size());
	slots_.emplace_back(itemId, count, gridX, gridY, rotated);
	OccupyGrid(slotIndex, data, gridX, gridY, rotated);
	return true;
}

bool Inventory::RemoveItem(int slotIndex, int count)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return false;

	InventorySlot& slot = slots_[slotIndex];
	if (slot.IsEmpty()) return false;

	slot.stackCount -= count;
	if (slot.stackCount <= 0)
	{
		RemoveSlot(slotIndex);
	}

	return true;
}

void Inventory::RemoveSlot(int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return;

	ReleaseGrid(slotIndex);
	slots_[slotIndex].itemId = 0;
	slots_[slotIndex].stackCount = 0;
}

int Inventory::AddToStack(int slotIndex, int count)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return 0;

	InventorySlot& slot = slots_[slotIndex];
	if (slot.IsEmpty()) return 0;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
	if (!data) return 0;

	int canAdd = data->maxStack - slot.stackCount;
	int toAdd = (count < canAdd) ? count : canAdd;

	if (toAdd > 0)
	{
		slot.stackCount += toAdd;
	}

	return toAdd;
}

bool Inventory::TryMergeSlots(int fromSlotIndex, int toSlotIndex)
{
	if (fromSlotIndex < 0 || fromSlotIndex >= static_cast<int>(slots_.size()))
		return false;
	if (toSlotIndex < 0 || toSlotIndex >= static_cast<int>(slots_.size()))
		return false;
	if (fromSlotIndex == toSlotIndex)
		return false;

	InventorySlot& fromSlot = slots_[fromSlotIndex];
	InventorySlot& toSlot = slots_[toSlotIndex];

	if (fromSlot.IsEmpty() || toSlot.IsEmpty())
		return false;

	// 同じアイテムでないとマージできない
	if (fromSlot.itemId != toSlot.itemId)
		return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(fromSlot.itemId);
	if (!data) return false;

	// 最大スタック数を確認
	int canAdd = data->maxStack - toSlot.stackCount;
	if (canAdd <= 0)
		return false;  // 既に満タン

	int toMove = (fromSlot.stackCount < canAdd) ? fromSlot.stackCount : canAdd;

	toSlot.stackCount += toMove;
	fromSlot.stackCount -= toMove;

	// fromSlotが空になったら削除
	if (fromSlot.stackCount <= 0)
	{
		RemoveSlot(fromSlotIndex);
	}

	return true;
}

bool Inventory::MoveItem(int slotIndex, int newGridX, int newGridY, bool newRotated)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return false;

	InventorySlot& slot = slots_[slotIndex];
	if (slot.IsEmpty()) return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
	if (!data) return false;

	// 現在位置・回転状態と同じなら何もしない
	if (slot.gridX == newGridX && slot.gridY == newGridY && slot.isRotated == newRotated)
		return true;

	// 配置可能か確認（自分自身は除外）
	if (!CanPlaceItem(data, newGridX, newGridY, newRotated, slotIndex))
		return false;

	// 移動・回転実行
	ReleaseGrid(slotIndex);
	slot.gridX = newGridX;
	slot.gridY = newGridY;
	slot.isRotated = newRotated;
	OccupyGrid(slotIndex, data, newGridX, newGridY, newRotated);

	return true;
}

bool Inventory::MoveItem(int slotIndex, int newGridX, int newGridY)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return false;
	return MoveItem(slotIndex, newGridX, newGridY, slots_[slotIndex].isRotated);
}

bool Inventory::RotateItem(int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return false;

	InventorySlot& slot = slots_[slotIndex];
	if (slot.IsEmpty()) return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
	if (!data) return false;

	// 正方形アイテムは回転不要
	if (data->gridWidth == data->gridHeight)
		return true;

	bool newRotated = !slot.isRotated;

	// 回転後に現在位置に置けるか確認
	if (!CanPlaceItem(data, slot.gridX, slot.gridY, newRotated, slotIndex))
		return false;

	// 回転実行
	ReleaseGrid(slotIndex);
	slot.isRotated = newRotated;
	OccupyGrid(slotIndex, data, slot.gridX, slot.gridY, newRotated);

	return true;
}

bool Inventory::CanPlaceItemAt(int slotIndex, int gridX, int gridY, bool rotated) const
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(slots_.size()))
		return false;

	const InventorySlot& slot = slots_[slotIndex];
	if (slot.IsEmpty()) return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
	return CanPlaceItem(data, gridX, gridY, rotated, slotIndex);
}

const InventorySlot* Inventory::GetSlotAt(int gridX, int gridY) const
{
	int index = GetSlotIndexAt(gridX, gridY);
	if (index < 0) return nullptr;
	return &slots_[index];
}

int Inventory::GetSlotIndexAt(int gridX, int gridY) const
{
	if (gridX < 0 || gridX >= gridWidth_ || gridY < 0 || gridY >= gridHeight_)
		return -1;
	return grid_[gridY][gridX];
}

const InventorySlot* Inventory::GetSlot(int index) const
{
	if (index < 0 || index >= static_cast<int>(slots_.size()))
		return nullptr;
	return &slots_[index];
}

InventorySlot* Inventory::GetSlot(int index)
{
	if (index < 0 || index >= static_cast<int>(slots_.size()))
		return nullptr;
	return &slots_[index];
}

float Inventory::GetTotalWeight() const
{
	float total = 0.0f;
	for (const auto& slot : slots_)
	{
		if (slot.IsEmpty()) continue;
		const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
		if (data)
		{
			total += data->weight * slot.stackCount;
		}
	}
	return total;
}

bool Inventory::CanFitItem(int itemId) const
{
	const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
	if (!data) return false;

	int x, y;
	if (FindEmptySpace(data, x, y, false)) return true;
	if (FindEmptySpace(data, x, y, true)) return true;
	return false;
}

bool Inventory::CanFitItemAt(int itemId, int gridX, int gridY, bool rotated) const
{
	const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
	if (!data) return false;

	return CanPlaceItem(data, gridX, gridY, rotated);
}

bool Inventory::IsCellEmpty(int gridX, int gridY, int excludeSlotIndex) const
{
	if (gridX < 0 || gridX >= gridWidth_ || gridY < 0 || gridY >= gridHeight_)
		return false;
	int cellSlot = grid_[gridY][gridX];
	if (cellSlot == -1) return true;
	if (excludeSlotIndex >= 0 && cellSlot == excludeSlotIndex) return true;
	return false;
}

bool Inventory::CanPlaceItem(const ItemData* data, int gridX, int gridY, bool rotated, int excludeSlotIndex) const
{
	if (!data) return false;

	int w = GetItemWidth(data, rotated);
	int h = GetItemHeight(data, rotated);

	// 範囲チェック
	if (gridX < 0 || gridX + w > gridWidth_) return false;
	if (gridY < 0 || gridY + h > gridHeight_) return false;

	// 全セルが空いているかチェック
	for (int dy = 0; dy < h; ++dy)
	{
		for (int dx = 0; dx < w; ++dx)
		{
			if (!IsCellEmpty(gridX + dx, gridY + dy, excludeSlotIndex))
				return false;
		}
	}

	return true;
}

bool Inventory::FindEmptySpace(const ItemData* data, int& outX, int& outY, bool rotated) const
{
	if (!data) return false;

	int w = GetItemWidth(data, rotated);
	int h = GetItemHeight(data, rotated);

	for (int y = 0; y <= gridHeight_ - h; ++y)
	{
		for (int x = 0; x <= gridWidth_ - w; ++x)
		{
			if (CanPlaceItem(data, x, y, rotated))
			{
				outX = x;
				outY = y;
				return true;
			}
		}
	}
	return false;
}

void Inventory::OccupyGrid(int slotIndex, const ItemData* data, int gridX, int gridY, bool rotated)
{
	if (!data) return;

	int w = GetItemWidth(data, rotated);
	int h = GetItemHeight(data, rotated);

	for (int dy = 0; dy < h; ++dy)
	{
		for (int dx = 0; dx < w; ++dx)
		{
			grid_[gridY + dy][gridX + dx] = slotIndex;
		}
	}
}

void Inventory::ReleaseGrid(int slotIndex)
{
	for (int y = 0; y < gridHeight_; ++y)
	{
		for (int x = 0; x < gridWidth_; ++x)
		{
			if (grid_[y][x] == slotIndex)
			{
				grid_[y][x] = -1;
			}
		}
	}
}

int Inventory::GetItemWidth(const ItemData* data, bool rotated) const
{
	if (!data) return 1;
	return rotated ? data->gridHeight : data->gridWidth;
}

int Inventory::GetItemHeight(const ItemData* data, bool rotated) const
{
	if (!data) return 1;
	return rotated ? data->gridWidth : data->gridHeight;
}

void Inventory::Resize(int newWidth, int newHeight)
{
	// 新しいサイズが小さい場合、はみ出すアイテムを削除する必要がある
	// ここでは単純に新しいグリッドを作成し、既存アイテムを再配置

	// 古いグリッドをバックアップ
	std::vector<InventorySlot> oldSlots = slots_;

	// 新しいサイズに変更
	gridWidth_ = newWidth;
	gridHeight_ = newHeight;

	// グリッドをクリアして再初期化
	grid_.clear();
	grid_.resize(gridHeight_, std::vector<int>(gridWidth_, -1));
	slots_.clear();

	// 既存アイテムを再配置
	for (const auto& slot : oldSlots)
	{
		if (!slot.IsEmpty())
		{
			// 元の位置に配置を試みる
			if (!AddItemAt(slot.itemId, slot.stackCount, slot.gridX, slot.gridY, slot.isRotated))
			{
				// 元の位置が使えない場合、自動配置
				AddItem(slot.itemId, slot.stackCount);
			}
		}
	}
}

void Inventory::SetQuickSlotCount(int count)
{
	quickSlotCount_ = count;
	quickSlots_.resize(count, 0);
}

int Inventory::GetQuickSlotItem(int slot) const
{
	if (slot < 0 || slot >= static_cast<int>(quickSlots_.size()))
		return 0;
	return quickSlots_[slot];
}

void Inventory::SetQuickSlotItem(int slot, int itemId)
{
	if (slot < 0 || slot >= static_cast<int>(quickSlots_.size()))
		return;
	quickSlots_[slot] = itemId;
}

void Inventory::ClearQuickSlot(int slot)
{
	if (slot < 0 || slot >= static_cast<int>(quickSlots_.size()))
		return;
	quickSlots_[slot] = 0;
}

bool Inventory::UseQuickSlotItem(int slot)
{
	if (slot < 0 || slot >= static_cast<int>(quickSlots_.size()))
		return false;

	int itemId = quickSlots_[slot];
	if (itemId == 0)
		return false;

	// インベントリからアイテムを探して使用
	for (int i = 0; i < static_cast<int>(slots_.size()); ++i)
	{
		if (slots_[i].itemId == itemId)
		{
			// アイテムを1つ消費
			RemoveItem(i, 1);

			// スロットにまだ同じアイテムがあるか確認
			bool stillHas = false;
			for (const auto& s : slots_)
			{
				if (s.itemId == itemId)
				{
					stillHas = true;
					break;
				}
			}
			// なければクイックスロットをクリア
			if (!stillHas)
			{
				ClearQuickSlot(slot);
			}
			return true;
		}
	}

	return false;
}
