#include "LootContainer.h"
#include "Data/ItemDatabase.h"
#include "System/Resource/ResourceManager.h"
#include <DxLib.h>

LootContainer::LootContainer(ContainerType type, const Vector2& pos)
	: type_(type)
	, position_(pos)
	, interactRadius_(30.0f)
	, isLooted_(false)
	, contents_(4, 3)  // 4x3の小さいインベントリ
{
}

void LootContainer::GenerateLoot(std::mt19937& rng)
{
	// タイプに応じたデフォルトのルートテーブルを使用
	LootTable table;
	switch (type_)
	{
	case ContainerType::Crate:
		table = LootTables::CreateCrateLoot();
		break;
	case ContainerType::Barrel:
		table = LootTables::CreateBarrelLoot();
		break;
	case ContainerType::WeaponCrate:
		table = LootTables::CreateWeaponCrateLoot();
		break;
	case ContainerType::Treasure:
		table = LootTables::CreateTreasureChestLoot();
		break;
	case ContainerType::Corpse:
		table = LootTables::CreateEnemyCorpseLoot();
		break;
	case ContainerType::GearCrate:
		table = LootTables::CreateGearCrateLoot();
		break;
	case ContainerType::RangedCrate:
		table = LootTables::CreateRangedCrateLoot();
		break;
	case ContainerType::ArmorCrate:
		table = LootTables::CreateArmorCrateLoot();
		break;
	case ContainerType::MedicalCrate:
		table = LootTables::CreateMedicalCrateLoot();
		break;
	default:
		return;
	}

	GenerateLoot(table, rng);
}

void LootContainer::GenerateLoot(const LootTable& table, std::mt19937& rng)
{
	auto loot = table.GenerateLoot(rng);

	for (const auto& item : loot)
	{
		contents_.AddItem(item.first, item.second);
	}
}

void LootContainer::Draw(const Vector2& cameraOffset) const
{
	int ox = static_cast<int>(cameraOffset.x);
	int oy = static_cast<int>(cameraOffset.y);
	int x = static_cast<int>(position_.x) - ox;
	int y = static_cast<int>(position_.y) - oy;

	auto& res = ResourceManager::GetInstance();
	const char* imgKey = nullptr;
	const char* imgKeyOpen = nullptr;

	switch (type_)
	{
	case ContainerType::Crate:
		imgKey = "container_crate";
		imgKeyOpen = "container_crate_open";
		break;
	case ContainerType::Barrel:
		imgKey = "container_barrel";
		break;
	case ContainerType::WeaponCrate:
		imgKey = "container_weapon_crate";
		break;
	case ContainerType::Treasure:
		imgKey = "container_treasure";
		imgKeyOpen = "container_treasure_open";
		break;
	case ContainerType::Corpse:
		imgKey = "container_corpse";
		break;
	default:
		break;
	}

	// 画像描画を試みる
	int handle = -1;
	if (isLooted_ && imgKeyOpen)
	{
		handle = res.GetGraph(imgKeyOpen);
	}
	if (handle == -1 && imgKey)
	{
		handle = res.GetGraph(imgKey);
	}

	if (handle != -1)
	{
		int imgW, imgH;
		GetGraphSize(handle, &imgW, &imgH);

		// 開いた状態は暗く表示
		if (isLooted_)
		{
			SetDrawBright(150, 150, 150);
		}
		DrawGraph(x - imgW / 2, y - imgH / 2, handle, TRUE);
		if (isLooted_)
		{
			SetDrawBright(255, 255, 255);
		}
	}
	else
	{
		// フォールバック: デバッグ描画
		unsigned int color;
		int size = 16;

		switch (type_)
		{
		case ContainerType::Crate:
			color = isLooted_ ? GetColor(80, 60, 40) : GetColor(139, 90, 43);
			DrawBox(x - size, y - size, x + size, y + size, color, TRUE);
			DrawBox(x - size, y - size, x + size, y + size, GetColor(100, 70, 30), FALSE);
			break;

		case ContainerType::Barrel:
			color = isLooted_ ? GetColor(60, 50, 40) : GetColor(120, 80, 40);
			DrawCircle(x, y, size, color, TRUE);
			DrawCircle(x, y, size, GetColor(80, 50, 20), FALSE);
			break;

		case ContainerType::WeaponCrate:
			color = isLooted_ ? GetColor(60, 60, 70) : GetColor(100, 100, 120);
			DrawBox(x - size, y - size / 2, x + size, y + size / 2, color, TRUE);
			DrawBox(x - size, y - size / 2, x + size, y + size / 2, GetColor(80, 80, 100), FALSE);
			if (!isLooted_)
			{
				DrawLine(x, y - 6, x, y + 6, GetColor(200, 200, 200));
				DrawLine(x - 4, y - 2, x + 4, y - 2, GetColor(200, 200, 200));
			}
			break;

		case ContainerType::Treasure:
			color = isLooted_ ? GetColor(100, 80, 30) : GetColor(200, 160, 50);
			DrawBox(x - size, y - size / 2, x + size, y + size / 2, color, TRUE);
			DrawBox(x - size, y - size / 2, x + size, y + size / 2, GetColor(180, 140, 40), FALSE);
			if (!isLooted_)
			{
				DrawCircle(x, y, 4, GetColor(255, 100, 100), TRUE);
			}
			break;

		case ContainerType::Corpse:
			color = isLooted_ ? GetColor(80, 60, 60) : GetColor(120, 80, 80);
			DrawBox(x - 12, y - 6, x + 12, y + 6, color, TRUE);
			DrawCircle(x - 8, y, 6, color, TRUE);
			DrawBox(x - 12, y - 6, x + 12, y + 6, GetColor(80, 50, 50), FALSE);
			break;

		case ContainerType::GearCrate:
			color = isLooted_ ? GetColor(60, 80, 60) : GetColor(100, 140, 100);
			DrawBox(x - size, y - size, x + size, y + size, color, TRUE);
			DrawBox(x - size, y - size, x + size, y + size, GetColor(70, 100, 70), FALSE);
			if (!isLooted_)
			{
				DrawLine(x - 6, y, x + 6, y, GetColor(200, 200, 200));
				DrawLine(x, y - 6, x, y + 6, GetColor(200, 200, 200));
			}
			break;

		case ContainerType::RangedCrate:
			color = isLooted_ ? GetColor(70, 70, 90) : GetColor(110, 110, 150);
			DrawBox(x - size, y - size / 2, x + size, y + size / 2, color, TRUE);
			DrawBox(x - size, y - size / 2, x + size, y + size / 2, GetColor(90, 90, 120), FALSE);
			if (!isLooted_)
			{
				DrawLine(x - 10, y, x + 8, y, GetColor(200, 180, 150));  // Arrow shape
				DrawLine(x + 6, y - 3, x + 10, y, GetColor(200, 180, 150));
				DrawLine(x + 6, y + 3, x + 10, y, GetColor(200, 180, 150));
			}
			break;

		case ContainerType::ArmorCrate:
			color = isLooted_ ? GetColor(60, 60, 80) : GetColor(100, 100, 140);
			DrawBox(x - size, y - size, x + size, y + size, color, TRUE);
			DrawBox(x - size, y - size, x + size, y + size, GetColor(80, 80, 110), FALSE);
			if (!isLooted_)
			{
				// Shield shape
				DrawCircle(x, y - 2, 6, GetColor(180, 180, 200), FALSE);
			}
			break;

		case ContainerType::MedicalCrate:
			color = isLooted_ ? GetColor(80, 50, 50) : GetColor(150, 80, 80);
			DrawBox(x - size, y - size, x + size, y + size, color, TRUE);
			DrawBox(x - size, y - size, x + size, y + size, GetColor(200, 100, 100), FALSE);
			if (!isLooted_)
			{
				// Red cross
				DrawBox(x - 6, y - 2, x + 6, y + 2, GetColor(255, 255, 255), TRUE);
				DrawBox(x - 2, y - 6, x + 2, y + 6, GetColor(255, 255, 255), TRUE);
			}
			break;

		default:
			break;
		}
	}
}

bool LootContainer::IsInRange(const Vector2& playerPos, float playerRadius) const
{
	float distance = position_.Distance(playerPos);
	return distance <= interactRadius_ + playerRadius;
}

bool LootContainer::IsEmpty() const
{
	return contents_.GetSlots().empty() ||
		std::all_of(contents_.GetSlots().begin(), contents_.GetSlots().end(),
			[](const InventorySlot& slot) { return slot.IsEmpty(); });
}
