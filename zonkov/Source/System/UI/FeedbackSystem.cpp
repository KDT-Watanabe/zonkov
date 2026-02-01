#include "FeedbackSystem.h"
#include <DxLib.h>
#include <algorithm>

void FeedbackSystem::Update(float deltaTime)
{
	// ダメージポップアップ更新
	for (auto& popup : damagePopups_)
	{
		popup.lifetime += deltaTime;
		popup.position.y -= 40.0f * deltaTime;  // 上に浮かぶ
	}

	// 期限切れ削除
	damagePopups_.erase(
		std::remove_if(damagePopups_.begin(), damagePopups_.end(),
			[](const DamagePopup& p) { return p.IsExpired(); }),
		damagePopups_.end()
	);

	// 通知更新
	for (auto& notif : notifications_)
	{
		notif.lifetime += deltaTime;
	}

	notifications_.erase(
		std::remove_if(notifications_.begin(), notifications_.end(),
			[](const Notification& n) { return n.IsExpired(); }),
		notifications_.end()
	);

	// 画面エフェクト更新
	for (auto& effect : screenEffects_)
	{
		effect.lifetime += deltaTime;
	}

	screenEffects_.erase(
		std::remove_if(screenEffects_.begin(), screenEffects_.end(),
			[](const ScreenEffect& e) { return e.IsExpired(); }),
		screenEffects_.end()
	);
}

void FeedbackSystem::Draw(const Vector2& cameraOffset) const
{
	int ox = static_cast<int>(cameraOffset.x);
	int oy = static_cast<int>(cameraOffset.y);

	// ダメージポップアップ描画
	for (const auto& popup : damagePopups_)
	{
		int x = static_cast<int>(popup.position.x) - ox;
		int y = static_cast<int>(popup.position.y) - oy;
		int alpha = static_cast<int>(255 * popup.GetAlpha());

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

		unsigned int color;
		int fontSize = 16;

		if (popup.isHeal)
		{
			color = GetColor(50, 255, 50);
			DrawFormatString(x - 15, y, color, "+%d", popup.damage);
		}
		else if (popup.isCritical)
		{
			color = GetColor(255, 200, 50);
			fontSize = 20;
			DrawFormatString(x - 20, y, color, "%d!", popup.damage);
		}
		else
		{
			color = GetColor(255, 255, 255);
			DrawFormatString(x - 10, y, color, "%d", popup.damage);
		}

		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void FeedbackSystem::DrawScreenEffects() const
{
	for (const auto& effect : screenEffects_)
	{
		float intensity = effect.GetCurrentIntensity();
		int alpha = static_cast<int>(255 * intensity * 0.3f);

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

		unsigned int color;
		switch (effect.type)
		{
		case ScreenEffect::Type::Damage:
			color = GetColor(255, 0, 0);
			break;
		case ScreenEffect::Type::Heal:
			color = GetColor(0, 255, 100);
			break;
		case ScreenEffect::Type::LowHealth:
			color = GetColor(255, 50, 50);
			break;
		case ScreenEffect::Type::Flash:
			color = GetColor(255, 255, 255);
			break;
		default:
			color = GetColor(255, 255, 255);
			break;
		}

		// 画面端にオーバーレイ（ビネット風）
		int screenW = 1280;
		int screenH = 720;
		int borderSize = 60;

		// 上
		DrawBox(0, 0, screenW, borderSize, color, TRUE);
		// 下
		DrawBox(0, screenH - borderSize, screenW, screenH, color, TRUE);
		// 左
		DrawBox(0, 0, borderSize, screenH, color, TRUE);
		// 右
		DrawBox(screenW - borderSize, 0, screenW, screenH, color, TRUE);

		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void FeedbackSystem::DrawNotifications() const
{
	int y = NOTIFICATION_Y;

	for (size_t i = 0; i < notifications_.size() && i < MAX_NOTIFICATIONS; ++i)
	{
		const auto& notif = notifications_[i];
		int alpha = static_cast<int>(255 * notif.GetAlpha());

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

		// 背景
		int textWidth = static_cast<int>(notif.message.length() * 8);
		DrawBox(NOTIFICATION_X - 5, y - 2, NOTIFICATION_X + textWidth + 10, y + 18,
			GetColor(20, 20, 30), TRUE);

		// アイコン色
		unsigned int iconColor;
		const char* icon = "";
		switch (notif.type)
		{
		case NotificationType::ItemPickup:
			iconColor = GetColor(100, 200, 100);
			icon = "+";
			break;
		case NotificationType::ItemUse:
			iconColor = GetColor(100, 200, 255);
			icon = "*";
			break;
		case NotificationType::EquipChange:
			iconColor = GetColor(200, 200, 100);
			icon = "=";
			break;
		case NotificationType::Warning:
			iconColor = GetColor(255, 100, 100);
			icon = "!";
			break;
		case NotificationType::QuestUpdate:
			iconColor = GetColor(255, 200, 50);
			icon = ">";
			break;
		default:
			iconColor = GetColor(200, 200, 200);
			icon = "-";
			break;
		}

		DrawFormatString(NOTIFICATION_X, y, iconColor, "[%s]", icon);
		DrawString(NOTIFICATION_X + 25, y, notif.message.c_str(), GetColor(255, 255, 255));

		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		y -= NOTIFICATION_SPACING;  // 下から上へ積み上げ
	}
}

void FeedbackSystem::ShowDamage(const Vector2& worldPos, int damage, bool isCritical)
{
	// ランダムなオフセットを追加
	Vector2 pos = worldPos;
	pos.x += static_cast<float>((rand() % 20) - 10);
	pos.y -= 10.0f;

	damagePopups_.emplace_back(pos, damage, isCritical, false);
}

void FeedbackSystem::ShowHeal(const Vector2& worldPos, int amount)
{
	Vector2 pos = worldPos;
	pos.x += static_cast<float>((rand() % 20) - 10);
	pos.y -= 10.0f;

	damagePopups_.emplace_back(pos, amount, false, true);
}

void FeedbackSystem::ShowNotification(const std::string& message, NotificationType type)
{
	// 古い通知を削除して新しいものを追加
	if (notifications_.size() >= MAX_NOTIFICATIONS)
	{
		notifications_.erase(notifications_.begin());
	}
	notifications_.emplace_back(message, type);
}

void FeedbackSystem::ShowItemPickup(const std::string& itemName, int count)
{
	std::string msg = itemName;
	if (count > 1)
	{
		msg += " x" + std::to_string(count);
	}
	ShowNotification(msg, NotificationType::ItemPickup);
}

void FeedbackSystem::ShowItemUse(const std::string& itemName)
{
	ShowNotification("使用: " + itemName, NotificationType::ItemUse);
}

void FeedbackSystem::ShowEquipChange(const std::string& itemName, bool equipped)
{
	std::string msg = equipped ? "装備: " : "装備解除: ";
	msg += itemName;
	ShowNotification(msg, NotificationType::EquipChange);
}

void FeedbackSystem::ShowWarning(const std::string& message)
{
	ShowNotification(message, NotificationType::Warning);
}

void FeedbackSystem::TriggerDamageEffect(float intensity)
{
	screenEffects_.emplace_back(ScreenEffect::Type::Damage, intensity, 0.3f);
}

void FeedbackSystem::TriggerHealEffect(float intensity)
{
	screenEffects_.emplace_back(ScreenEffect::Type::Heal, intensity, 0.5f);
}

void FeedbackSystem::TriggerLowHealthWarning()
{
	// 既に警告がある場合は追加しない
	for (const auto& effect : screenEffects_)
	{
		if (effect.type == ScreenEffect::Type::LowHealth)
		{
			return;
		}
	}
	screenEffects_.emplace_back(ScreenEffect::Type::LowHealth, 0.3f, 1.0f);
}

void FeedbackSystem::Clear()
{
	damagePopups_.clear();
	notifications_.clear();
	screenEffects_.clear();
}
