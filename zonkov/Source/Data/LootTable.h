#pragma once
#include <vector>
#include <random>

// ルートエントリ（個別アイテムの出現確率）
struct LootEntry
{
	int itemId;      // アイテムID
	int minCount;    // 最小個数
	int maxCount;    // 最大個数
	float weight;    // 出現確率の重み（高いほど出やすい）

	LootEntry(int id, int minC, int maxC, float w)
		: itemId(id), minCount(minC), maxCount(maxC), weight(w) {}
};

// ルートテーブル（コンテナ用）
struct LootTable
{
	std::vector<LootEntry> entries;
	int minItems;    // 最小アイテム種類数
	int maxItems;    // 最大アイテム種類数

	LootTable() : minItems(1), maxItems(3) {}

	// 重み付きランダムでアイテムを選択
	// 戻り値: vector<pair<itemId, count>>
	std::vector<std::pair<int, int>> GenerateLoot(std::mt19937& rng) const
	{
		std::vector<std::pair<int, int>> result;

		if (entries.empty()) return result;

		// アイテム種類数を決定
		std::uniform_int_distribution<int> countDist(minItems, maxItems);
		int itemCount = countDist(rng);

		// 重みの合計を計算
		float totalWeight = 0.0f;
		for (const auto& entry : entries)
		{
			totalWeight += entry.weight;
		}

		if (totalWeight <= 0.0f) return result;

		// 選択済みアイテムを追跡
		std::vector<bool> selected(entries.size(), false);

		for (int i = 0; i < itemCount && i < static_cast<int>(entries.size()); ++i)
		{
			// 重み付きランダム選択
			std::uniform_real_distribution<float> weightDist(0.0f, totalWeight);
			float roll = weightDist(rng);

			float cumulative = 0.0f;
			int selectedIndex = -1;

			for (size_t j = 0; j < entries.size(); ++j)
			{
				if (selected[j]) continue;

				cumulative += entries[j].weight;
				if (roll <= cumulative)
				{
					selectedIndex = static_cast<int>(j);
					break;
				}
			}

			if (selectedIndex < 0) continue;

			selected[selectedIndex] = true;
			totalWeight -= entries[selectedIndex].weight;

			// 個数を決定
			const LootEntry& entry = entries[selectedIndex];
			std::uniform_int_distribution<int> stackDist(entry.minCount, entry.maxCount);
			int count = stackDist(rng);

			result.emplace_back(entry.itemId, count);
		}

		return result;
	}
};

// 定義済みルートテーブル
namespace LootTables
{
	// 木箱用（一般的なアイテム）
	inline LootTable CreateCrateLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 3;
		table.entries = {
			LootEntry(1, 1, 2, 1.0f),    // Small Potion
			LootEntry(3, 1, 3, 0.8f),    // Bandage
			LootEntry(30, 1, 5, 0.6f),   // Iron Ore
			LootEntry(31, 2, 6, 0.7f),   // Herb
		};
		return table;
	}

	// 樽用（素材中心）
	inline LootTable CreateBarrelLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 2;
		table.entries = {
			LootEntry(30, 2, 8, 1.0f),   // Iron Ore
			LootEntry(31, 3, 10, 0.9f),  // Herb
			LootEntry(1, 1, 1, 0.3f),    // Small Potion
		};
		return table;
	}

	// 武器箱用（装備品）
	inline LootTable CreateWeaponCrateLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 2;
		table.entries = {
			LootEntry(10, 1, 1, 0.5f),   // Short Sword
			LootEntry(12, 1, 1, 0.6f),   // Dagger
			LootEntry(11, 1, 1, 0.2f),   // Long Sword (rare)
			LootEntry(22, 1, 1, 0.4f),   // Wooden Shield
		};
		return table;
	}

	// 宝箱用（貴重品）
	inline LootTable CreateTreasureChestLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 3;
		table.entries = {
			LootEntry(40, 1, 3, 0.8f),   // Gold Ring
			LootEntry(41, 2, 5, 0.7f),   // Ancient Coin
			LootEntry(42, 1, 1, 0.1f),   // Jeweled Crown (very rare)
			LootEntry(2, 1, 2, 0.4f),    // Large Potion
		};
		return table;
	}

	// 敵の死体用（少なめ）
	inline LootTable CreateEnemyCorpseLoot()
	{
		LootTable table;
		table.minItems = 0;
		table.maxItems = 2;
		table.entries = {
			LootEntry(1, 1, 1, 0.3f),    // Small Potion
			LootEntry(41, 1, 3, 0.5f),   // Ancient Coin
			LootEntry(3, 1, 2, 0.4f),    // Bandage
		};
		return table;
	}

	// ギア（バックパック・リグ）用
	inline LootTable CreateGearCrateLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 2;
		table.entries = {
			LootEntry(50, 1, 1, 0.5f),   // Small Backpack
			LootEntry(51, 1, 1, 0.3f),   // Medium Backpack
			LootEntry(52, 1, 1, 0.1f),   // Large Backpack (rare)
			LootEntry(60, 1, 1, 0.5f),   // Light Rig
			LootEntry(61, 1, 1, 0.3f),   // Tactical Rig
			LootEntry(62, 1, 1, 0.1f),   // Heavy Rig (rare)
		};
		return table;
	}

	// 遠距離武器用
	inline LootTable CreateRangedCrateLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 3;
		table.entries = {
			LootEntry(13, 1, 1, 0.5f),   // Short Bow
			LootEntry(14, 1, 1, 0.2f),   // Long Bow (rare)
			LootEntry(15, 1, 1, 0.3f),   // Magic Staff
			LootEntry(16, 1, 1, 0.15f),  // Fire Wand (rare)
			LootEntry(70, 5, 10, 0.8f),  // Arrow
			LootEntry(71, 3, 8, 0.6f),   // Mana Crystal
		};
		return table;
	}

	// 防具用
	inline LootTable CreateArmorCrateLoot()
	{
		LootTable table;
		table.minItems = 1;
		table.maxItems = 2;
		table.entries = {
			LootEntry(20, 1, 1, 0.5f),   // Leather Armor
			LootEntry(21, 1, 1, 0.3f),   // Chainmail
			LootEntry(22, 1, 1, 0.4f),   // Wooden Shield
			LootEntry(3, 1, 3, 0.3f),    // Bandage
		};
		return table;
	}

	// 医療品用
	inline LootTable CreateMedicalCrateLoot()
	{
		LootTable table;
		table.minItems = 2;
		table.maxItems = 4;
		table.entries = {
			LootEntry(1, 1, 2, 0.8f),    // Small Potion
			LootEntry(2, 1, 1, 0.4f),    // Large Potion
			LootEntry(3, 2, 5, 0.9f),    // Bandage
			LootEntry(31, 2, 5, 0.6f),   // Herb
		};
		return table;
	}
}
