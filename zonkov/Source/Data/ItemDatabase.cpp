#include "ItemDatabase.h"

void ItemDatabase::Initialize()
{
	if (isInitialized_) return;
	isInitialized_ = true;

	items_.clear();

	// テスト用アイテム登録
	// ID, 名前, タイプ, 横サイズ, 縦サイズ, 重量, 最大スタック, 説明, 回復量

	// 消費アイテム（回復量付き）
	RegisterItem(ItemData(1, "小回復薬", ItemType::Consumable, 1, 1, 0.2f, 3, "HPを25回復", 25));
	RegisterItem(ItemData(2, "大回復薬", ItemType::Consumable, 1, 2, 0.4f, 2, "HPを50回復", 50));
	RegisterItem(ItemData(3, "包帯", ItemType::Consumable, 1, 1, 0.1f, 5, "HPを10回復", 10));

	// 近接武器
	RegisterItem(ItemData(10, "ショートソード", ItemType::Weapon, 1, 3, 1.5f, 1, "基本的な斬撃武器"));
	RegisterItem(ItemData(11, "ロングソード", ItemType::Weapon, 1, 4, 2.5f, 1, "強力な斬撃武器"));
	RegisterItem(ItemData(12, "ダガー", ItemType::Weapon, 1, 2, 0.5f, 1, "素早い刺突武器"));

	// 遠距離武器
	RegisterItem(ItemData(13, "ショートボウ", ItemType::Weapon, 1, 3, 1.0f, 1, "基本的な遠距離武器"));
	RegisterItem(ItemData(14, "ロングボウ", ItemType::Weapon, 2, 4, 1.8f, 1, "強力な遠距離武器"));
	RegisterItem(ItemData(15, "魔法の杖", ItemType::Weapon, 1, 4, 1.2f, 1, "魔力を放つ"));
	RegisterItem(ItemData(16, "炎の杖", ItemType::Weapon, 1, 3, 0.8f, 1, "火球を発射"));

	// 弾薬
	RegisterItem(ItemData(70, "矢", ItemType::Material, 1, 1, 0.05f, 10, "弓の弾薬"));
	RegisterItem(ItemData(71, "マナクリスタル", ItemType::Material, 1, 1, 0.1f, 8, "魔法の弾薬"));

	// 防具
	RegisterItem(ItemData(20, "レザーアーマー", ItemType::Armor, 2, 3, 3.0f, 1, "軽量でバランスの良い防具"));
	RegisterItem(ItemData(21, "チェインメイル", ItemType::Armor, 2, 3, 5.0f, 1, "高い防御力の重装備"));
	RegisterItem(ItemData(22, "木の盾", ItemType::Armor, 2, 2, 2.5f, 1, "基本的な盾"));

	// 素材
	RegisterItem(ItemData(30, "鉄鉱石", ItemType::Material, 1, 1, 0.5f, 8, "加工用の鉄鉱石"));
	RegisterItem(ItemData(31, "薬草", ItemType::Material, 1, 1, 0.1f, 10, "薬の材料"));

	// 貴重品
	RegisterItem(ItemData(40, "金の指輪", ItemType::Valuable, 1, 1, 0.05f, 5, "価値のある宝飾品"));
	RegisterItem(ItemData(41, "古代のコイン", ItemType::Valuable, 1, 1, 0.02f, 20, "希少なコレクターアイテム"));
	RegisterItem(ItemData(42, "宝石の王冠", ItemType::Valuable, 2, 2, 0.8f, 1, "王族の至宝"));

	// バックパック
	RegisterItem(ItemData(50, "小型バックパック", ItemType::Backpack, 2, 2, 0.5f, 1, "容量+2x2"));
	RegisterItem(ItemData(51, "中型バックパック", ItemType::Backpack, 2, 3, 1.0f, 1, "容量+3x3"));
	RegisterItem(ItemData(52, "大型バックパック", ItemType::Backpack, 3, 3, 1.5f, 1, "容量+4x4 移動速度-10%"));

	// リグ
	RegisterItem(ItemData(60, "ライトリグ", ItemType::Rig, 2, 2, 0.3f, 1, "クイックスロット2個"));
	RegisterItem(ItemData(61, "タクティカルリグ", ItemType::Rig, 2, 3, 0.6f, 1, "クイックスロット4個+収納1x2"));
	RegisterItem(ItemData(62, "ヘビーリグ", ItemType::Rig, 3, 3, 1.0f, 1, "クイックスロット6個+収納2x2"));
}

const ItemData* ItemDatabase::GetItemData(int itemId) const
{
	for (const auto& item : items_)
	{
		if (item.id == itemId)
		{
			return &item;
		}
	}
	return nullptr;
}

void ItemDatabase::RegisterItem(const ItemData& data)
{
	items_.push_back(data);
}
