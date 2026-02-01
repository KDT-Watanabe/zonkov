#include "DxLib.h"
#include "ResourceManager.h"

void ResourceManager::LoadAllAssets()
{
	if (isLoaded_) return;
	isLoaded_ = true;

	// Base path for assets
	const std::string basePath = "Assets/";

	// Characters
	LoadGraph("player", basePath + "Characters/player.png");
	LoadGraph("player_attack", basePath + "Characters/player_attack.png");
	LoadGraph("enemy_melee", basePath + "Characters/enemy_melee.png");
	LoadGraph("enemy_melee_aggro", basePath + "Characters/enemy_melee_aggro.png");

	// Map
	LoadGraph("floor", basePath + "Map/floor.png");
	LoadGraph("wall", basePath + "Map/wall.png");
	LoadGraph("exit_area", basePath + "Map/exit_area.png");

	// Items (using item ID as key suffix)
	LoadGraph("item_1", basePath + "Items/item_small_potion.png");
	LoadGraph("item_2", basePath + "Items/item_large_potion.png");
	LoadGraph("item_3", basePath + "Items/item_bandage.png");
	LoadGraph("item_10", basePath + "Items/item_short_sword.png");
	LoadGraph("item_11", basePath + "Items/item_long_sword.png");
	LoadGraph("item_12", basePath + "Items/item_dagger.png");
	LoadGraph("item_20", basePath + "Items/item_leather_armor.png");
	LoadGraph("item_21", basePath + "Items/item_chainmail.png");
	LoadGraph("item_22", basePath + "Items/item_wooden_shield.png");
	LoadGraph("item_30", basePath + "Items/item_iron_ore.png");
	LoadGraph("item_31", basePath + "Items/item_herb.png");
	LoadGraph("item_40", basePath + "Items/item_gold_ring.png");
	LoadGraph("item_41", basePath + "Items/item_ancient_coin.png");
	LoadGraph("item_42", basePath + "Items/item_jeweled_crown.png");

	// Containers
	LoadGraph("container_crate", basePath + "Containers/container_crate.png");
	LoadGraph("container_crate_open", basePath + "Containers/container_crate_open.png");
	LoadGraph("container_barrel", basePath + "Containers/container_barrel.png");
	LoadGraph("container_weapon_crate", basePath + "Containers/container_weapon_crate.wpng");
	LoadGraph("container_treasure", basePath + "Containers/container_treasure.png");
	LoadGraph("container_treasure_open", basePath + "Containers/container_treasure_open.png");
	LoadGraph("container_corpse", basePath + "Containers/container_corpse.png");

	// UI
	LoadGraph("ui_inventory_bg", basePath + "UI/ui_inventory_bg.png");
	LoadGraph("ui_container_bg", basePath + "UI/ui_container_bg.png");
	LoadGraph("ui_slot", basePath + "UI/ui_slot.png");
	LoadGraph("ui_hp_bar_bg", basePath + "UI/ui_hp_bar_bg.png");
	LoadGraph("ui_hp_bar_fill", basePath + "UI/ui_hp_bar_fill.png");
	LoadGraph("ui_equip_slot", basePath + "UI/ui_equip_slot.png");
}

bool ResourceManager::HasGraph(const std::string& key) const
{
	auto it = graphHandles_.find(key);
	if (it != graphHandles_.end())
	{
		return it->second != -1;
	}
	return false;
}

int ResourceManager::LoadGraph(const std::string& key, const std::string& filePath)
{
	auto it = graphHandles_.find(key);
	if (it != graphHandles_.end())
	{
		return it->second;
	}

	int handle = ::LoadGraph(filePath.c_str());
	graphHandles_[key] = handle;
	return handle;
}

int ResourceManager::GetGraph(const std::string& key) const
{
	auto it = graphHandles_.find(key);
	if (it != graphHandles_.end())
	{
		return it->second;
	}
	return -1;
}

int ResourceManager::LoadDivGraph(const std::string& key, const std::string& filePath,
	int divX, int divY, int divWidth, int divHeight)
{
	auto it = divGraphHandles_.find(key);
	if (it != divGraphHandles_.end())
	{
		return it->second[0];
	}

	int totalCount = divX * divY;
	std::vector<int> handles(totalCount);
	::LoadDivGraph(filePath.c_str(), totalCount, divX, divY, divWidth, divHeight, handles.data());
	divGraphHandles_[key] = handles;
	return handles[0];
}

const std::vector<int>& ResourceManager::GetDivGraph(const std::string& key) const
{
	static std::vector<int> empty;
	auto it = divGraphHandles_.find(key);
	if (it != divGraphHandles_.end())
	{
		return it->second;
	}
	return empty;
}

int ResourceManager::LoadSound(const std::string& key, const std::string& filePath)
{
	auto it = soundHandles_.find(key);
	if (it != soundHandles_.end())
	{
		return it->second;
	}

	int handle = LoadSoundMem(filePath.c_str());
	soundHandles_[key] = handle;
	return handle;
}

int ResourceManager::GetSound(const std::string& key) const
{
	auto it = soundHandles_.find(key);
	if (it != soundHandles_.end())
	{
		return it->second;
	}
	return -1;
}

void ResourceManager::DeleteGraph(const std::string& key)
{
	auto it = graphHandles_.find(key);
	if (it != graphHandles_.end())
	{
		if (it->second != -1) ::DeleteGraph(it->second);
		graphHandles_.erase(it);
		return;
	}

	auto divIt = divGraphHandles_.find(key);
	if (divIt != divGraphHandles_.end())
	{
		for (int handle : divIt->second)
		{
			if (handle != -1) ::DeleteGraph(handle);
		}
		divGraphHandles_.erase(divIt);
	}
}

void ResourceManager::DeleteSound(const std::string& key)
{
	auto it = soundHandles_.find(key);
	if (it != soundHandles_.end())
	{
		if (it->second != -1) ::DeleteSoundMem(it->second);
		soundHandles_.erase(it);
	}
}

void ResourceManager::Clear()
{
	for (auto& pair : graphHandles_)
	{
		if (pair.second != -1) ::DeleteGraph(pair.second);
	}
	graphHandles_.clear();

	for (auto& pair : divGraphHandles_)
	{
		for (int handle : pair.second)
		{
			if (handle != -1) ::DeleteGraph(handle);
		}
	}
	divGraphHandles_.clear();

	for (auto& pair : soundHandles_)
	{
		if (pair.second != -1) ::DeleteSoundMem(pair.second);
	}
	soundHandles_.clear();
}
