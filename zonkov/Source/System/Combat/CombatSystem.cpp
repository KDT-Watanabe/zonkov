#include "CombatSystem.h"
#include "System/UI/FeedbackSystem.h"
#include "Entity/Player/Player.h"
#include "Entity/Enemy/Enemy.h"
#include "Data/ItemDatabase.h"
#include <cmath>

bool AttackHitbox::Intersects(const Vector2& targetPos, float targetRadius) const
{
	// 攻撃者からターゲットへのベクトル
	Vector2 toTarget = targetPos - origin;
	float distance = toTarget.GetLength();

	// 距離チェック
	if (distance > range + targetRadius)
	{
		return false;
	}

	// 方向チェック（弧の幅内か）
	if (distance > 0.0f)
	{
		Vector2 toTargetNorm = toTarget.Normalized();
		float dot = direction.Dot(toTargetNorm);
		float angleThreshold = std::cos((arcWidth * 0.5f) * 3.14159f / 180.0f);

		if (dot < angleThreshold)
		{
			return false;
		}
	}

	return true;
}

void CombatSystem::Initialize()
{
	if (isInitialized_) return;
	isInitialized_ = true;

	weapons_.clear();
	armors_.clear();
	backpacks_.clear();
	rigs_.clear();

	// 武器データ登録（ItemDatabaseのIDと対応）
	// ID, AttackType, BaseDamage, AttackSpeed, Range, AilmentChance

	// Short Sword (ID: 10) - 斬撃
	RegisterWeapon(WeaponData(10, AttackType::Slash, 15, 1.5f, 45.0f, 0.2f));

	// Long Sword (ID: 11) - 斬撃（高威力・遅い）
	RegisterWeapon(WeaponData(11, AttackType::Slash, 25, 1.0f, 55.0f, 0.25f));

	// Dagger (ID: 12) - 刺突（速い）
	RegisterWeapon(WeaponData(12, AttackType::Pierce, 10, 2.5f, 30.0f, 0.15f));

	// 遠距離武器
	// Short Bow (ID: 13)
	WeaponData shortBow(13, AttackType::Pierce, 12, 1.2f, 250.0f, 0.1f);
	shortBow.category = WeaponCategory::Bow;
	shortBow.projectileSpeed = 400.0f;
	shortBow.ammoItemId = 70;  // Arrow
	shortBow.ammoPerShot = 1;
	RegisterWeapon(shortBow);

	// Long Bow (ID: 14)
	WeaponData longBow(14, AttackType::Pierce, 20, 0.8f, 350.0f, 0.15f);
	longBow.category = WeaponCategory::Bow;
	longBow.projectileSpeed = 500.0f;
	longBow.ammoItemId = 70;  // Arrow
	longBow.ammoPerShot = 1;
	RegisterWeapon(longBow);

	// Magic Staff (ID: 15)
	WeaponData staff(15, AttackType::Blunt, 15, 1.0f, 280.0f, 0.2f);
	staff.category = WeaponCategory::Magic;
	staff.projectileSpeed = 350.0f;
	staff.ammoItemId = 71;  // Mana Crystal
	staff.ammoPerShot = 1;
	RegisterWeapon(staff);

	// Fire Wand (ID: 16)
	WeaponData fireWand(16, AttackType::Blunt, 25, 0.7f, 200.0f, 0.3f);
	fireWand.category = WeaponCategory::Magic;
	fireWand.projectileSpeed = 300.0f;
	fireWand.ammoItemId = 71;  // Mana Crystal
	fireWand.ammoPerShot = 2;
	RegisterWeapon(fireWand);

	// 防具データ登録
	// Leather Armor (ID: 20)
	ArmorData leather(20, false, 5);
	leather.resistances = DamageReduction(0.1f, 0.05f, 0.15f);
	RegisterArmor(leather);

	// Chainmail (ID: 21) - 重装防具
	ArmorData chainmail(21, false, 10);
	chainmail.resistances = DamageReduction(0.2f, 0.15f, 0.1f);
	RegisterArmor(chainmail);

	// Wooden Shield (ID: 22) - 基本盾
	ArmorData woodenShield(22, true, 3);
	woodenShield.resistances = DamageReduction(0.15f, 0.0f, 0.1f);
	woodenShield.conversion = AttributeConversion(AttackType::Slash, AttackType::Blunt, 0.3f);
	woodenShield.blockChance = 0.25f;
	RegisterArmor(woodenShield);

	// バックパックデータ登録
	// Small Backpack (ID: 50) - +2x2 容量
	RegisterBackpack(BackpackData(50, 2, 2, 0.0f));

	// Medium Backpack (ID: 51) - +3x3 容量
	RegisterBackpack(BackpackData(51, 3, 3, 0.0f));

	// Large Backpack (ID: 52) - +4x4 容量, -10%速度
	RegisterBackpack(BackpackData(52, 4, 4, 0.1f));

	// リグデータ登録
	// Light Rig (ID: 60) - 2クイックスロット
	RegisterRig(RigData(60, 2, 0, 0));

	// Tactical Rig (ID: 61) - 4クイックスロット + 1x2 追加容量
	RegisterRig(RigData(61, 4, 1, 2));

	// Heavy Rig (ID: 62) - 6クイックスロット + 2x2 追加容量
	RegisterRig(RigData(62, 6, 2, 2));
}

DamageResult CombatSystem::CalculateDamage(
	AttackType attackType,
	int baseDamage,
	float ailmentChance,
	const ArmorData* armor,
	const ArmorData* shield
) const
{
	DamageResult result;
	result.baseDamage = baseDamage;
	result.originalType = attackType;
	result.effectiveType = attackType;
	result.ailmentChance = ailmentChance;

	int damage = baseDamage;

	// 盾の属性変換
	if (shield)
	{
		result.effectiveType = ApplyShieldConversion(attackType, shield);
	}

	// 盾の防御
	if (shield)
	{
		damage = ApplyDamageReduction(damage, result.effectiveType, shield);
	}

	// 防具の防御
	if (armor)
	{
		damage = ApplyDamageReduction(damage, result.effectiveType, armor);
	}

	// 最低ダメージ保証
	if (damage < 1)
	{
		damage = 1;
	}

	result.finalDamage = damage;
	return result;
}

void CombatSystem::ProcessPlayerAttack(Player& player, std::vector<std::unique_ptr<Enemy>>& enemies)
{
	if (!player.IsAttacking()) return;

	// 攻撃開始フレームのみ処理（Timer == Cooldown - epsilon）
	float attackStart = player.GetAttackTimer();
	// 攻撃判定は攻撃開始時のみ
	// (実際のゲームではアニメーションに合わせて調整)

	AttackHitbox hitbox;
	hitbox.origin = player.GetPosition();
	hitbox.direction = player.GetFacingDirection();
	hitbox.range = player.GetAttackRange();
	hitbox.arcWidth = 120.0f;  // 120度の弧

	for (auto& enemy : enemies)
	{
		if (!enemy->IsAlive()) continue;
		if (enemy->IsInvincible()) continue;  // 無敵中はスキップ

		if (hitbox.Intersects(enemy->GetPosition(), enemy->GetRadius()))
		{
			// ダメージ計算（敵の防具は今のところなし）
			DamageResult result = CalculateDamage(
				player.GetAttackType(),
				player.GetAttackDamage(),
				0.15f,
				nullptr,
				nullptr
			);

			enemy->TakeDamage(result.finalDamage);

			// ダメージポップアップ表示
			bool isCritical = (result.finalDamage >= player.GetAttackDamage() * 1.5f);
			FeedbackSystem::GetInstance().ShowDamage(enemy->GetPosition(), result.finalDamage, isCritical);
		}
	}
}

void CombatSystem::ProcessEnemyAttack(Enemy& enemy, Player& player)
{
	if (!enemy.IsAttacking()) return;
	if (!player.IsAlive()) return;

	AttackHitbox hitbox;
	hitbox.origin = enemy.GetPosition();
	hitbox.direction = (player.GetPosition() - enemy.GetPosition()).Normalized();
	hitbox.range = enemy.GetAttackRange();
	hitbox.arcWidth = 90.0f;

	if (hitbox.Intersects(player.GetPosition(), player.GetRadius()))
	{
		// 無敵中はダメージ処理をスキップ
		if (player.IsInvincible())
		{
			return;
		}

		// プレイヤーの装備から防具データ取得
		const ArmorData* armor = GetArmorData(player.GetEquippedItemId(EquipSlot::Armor));
		const ArmorData* shield = GetArmorData(player.GetEquippedItemId(EquipSlot::Shield));

		DamageResult result = CalculateDamage(
			enemy.GetAttackType(),
			enemy.GetAttackDamage(),
			0.2f,
			armor,
			shield
		);

		player.TakeDamage(result.finalDamage);

		// ダメージポップアップ表示
		FeedbackSystem::GetInstance().ShowDamage(player.GetPosition(), result.finalDamage, false);

		// 画面エフェクト
		float intensity = static_cast<float>(result.finalDamage) / static_cast<float>(player.GetMaxHP());
		FeedbackSystem::GetInstance().TriggerDamageEffect(intensity);

		// 低HP警告
		if (player.GetHP() < player.GetMaxHP() * 0.3f)
		{
			FeedbackSystem::GetInstance().TriggerLowHealthWarning();
		}
	}
}

const WeaponData* CombatSystem::GetWeaponData(int itemId) const
{
	for (const auto& weapon : weapons_)
	{
		if (weapon.itemId == itemId)
		{
			return &weapon;
		}
	}
	return nullptr;
}

const ArmorData* CombatSystem::GetArmorData(int itemId) const
{
	for (const auto& armor : armors_)
	{
		if (armor.itemId == itemId)
		{
			return &armor;
		}
	}
	return nullptr;
}

void CombatSystem::RegisterWeapon(const WeaponData& data)
{
	weapons_.push_back(data);
}

void CombatSystem::RegisterArmor(const ArmorData& data)
{
	armors_.push_back(data);
}

void CombatSystem::RegisterBackpack(const BackpackData& data)
{
	backpacks_.push_back(data);
}

void CombatSystem::RegisterRig(const RigData& data)
{
	rigs_.push_back(data);
}

const BackpackData* CombatSystem::GetBackpackData(int itemId) const
{
	for (const auto& bp : backpacks_)
	{
		if (bp.itemId == itemId)
		{
			return &bp;
		}
	}
	return nullptr;
}

const RigData* CombatSystem::GetRigData(int itemId) const
{
	for (const auto& rig : rigs_)
	{
		if (rig.itemId == itemId)
		{
			return &rig;
		}
	}
	return nullptr;
}

EquipSlot CombatSystem::GetEquipSlotForItem(int itemId) const
{
	const ItemData* item = ItemDatabase::GetInstance().GetItem(itemId);
	if (!item) return EquipSlot::Count;

	if (item->type == ItemType::Weapon)
	{
		return EquipSlot::Weapon;
	}

	if (item->type == ItemType::Armor)
	{
		const ArmorData* armor = GetArmorData(itemId);
		if (armor && armor->isShield)
		{
			return EquipSlot::Shield;
		}
		return EquipSlot::Armor;
	}

	if (item->type == ItemType::Backpack)
	{
		return EquipSlot::Backpack;
	}

	if (item->type == ItemType::Rig)
	{
		return EquipSlot::Rig;
	}

	return EquipSlot::Count;
}

AttackType CombatSystem::ApplyShieldConversion(AttackType original, const ArmorData* shield) const
{
	if (!shield || !shield->isShield) return original;

	const AttributeConversion& conv = shield->conversion;
	if (conv.fromType == original && conv.conversionRate > 0.0f)
	{
		// 簡易版：変換率50%以上なら変換先を返す
		if (conv.conversionRate >= 0.5f)
		{
			return conv.toType;
		}
	}
	return original;
}

int CombatSystem::ApplyDamageReduction(int damage, AttackType type, const ArmorData* armor) const
{
	if (!armor) return damage;

	// 固定防御
	damage -= armor->baseDefense;

	// 属性耐性
	float reduction = armor->resistances.GetReduction(type);
	damage = static_cast<int>(damage * (1.0f - reduction));

	return damage;
}
