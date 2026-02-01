#pragma once
#include "Data/CombatTypes.h"
#include "Data/WeaponData.h"
#include "Data/ArmorData.h"
#include "Data/GearData.h"
#include "Utility/Vector2.h"
#include <vector>
#include <memory>

class Player;
class Enemy;

// 攻撃判定用ヒットボックス
struct AttackHitbox
{
	Vector2 origin;      // 攻撃者の位置
	Vector2 direction;   // 攻撃方向
	float range;         // 攻撃範囲
	float arcWidth;      // 攻撃弧の幅（度）

	AttackHitbox()
		: origin(0.0f, 0.0f)
		, direction(1.0f, 0.0f)
		, range(40.0f)
		, arcWidth(90.0f)
	{}

	// ターゲットに当たるかチェック
	bool Intersects(const Vector2& targetPos, float targetRadius) const;
};

// 戦闘システム（シングルトン）
class CombatSystem
{
public:
	static CombatSystem& GetInstance()
	{
		static CombatSystem instance;
		return instance;
	}

	// 初期化（武器・防具データ登録）
	void Initialize();

	// ダメージ計算
	DamageResult CalculateDamage(
		AttackType attackType,
		int baseDamage,
		float ailmentChance,
		const ArmorData* armor,
		const ArmorData* shield
	) const;

	// プレイヤーの攻撃処理
	void ProcessPlayerAttack(Player& player, std::vector<std::unique_ptr<Enemy>>& enemies);

	// 敵の攻撃処理
	void ProcessEnemyAttack(Enemy& enemy, Player& player);

	// 武器・防具・ギアデータ取得
	const WeaponData* GetWeaponData(int itemId) const;
	const ArmorData* GetArmorData(int itemId) const;
	const BackpackData* GetBackpackData(int itemId) const;
	const RigData* GetRigData(int itemId) const;

	// アイテムが装備可能なスロットを取得（装備不可ならEquipSlot::Countを返す）
	EquipSlot GetEquipSlotForItem(int itemId) const;

	// データ登録
	void RegisterWeapon(const WeaponData& data);
	void RegisterArmor(const ArmorData& data);
	void RegisterBackpack(const BackpackData& data);
	void RegisterRig(const RigData& data);

private:
	CombatSystem() : isInitialized_(false) {}
	~CombatSystem() = default;
	CombatSystem(const CombatSystem&) = delete;
	CombatSystem& operator=(const CombatSystem&) = delete;

	// 盾の属性変換適用
	AttackType ApplyShieldConversion(AttackType original, const ArmorData* shield) const;

	// 防具のダメージ軽減適用
	int ApplyDamageReduction(int damage, AttackType type, const ArmorData* armor) const;

private:
	bool isInitialized_;
	std::vector<WeaponData> weapons_;
	std::vector<ArmorData> armors_;
	std::vector<BackpackData> backpacks_;
	std::vector<RigData> rigs_;
};
