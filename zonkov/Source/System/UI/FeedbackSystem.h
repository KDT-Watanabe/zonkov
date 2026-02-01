#pragma once
#include "Utility/Vector2.h"
#include <vector>
#include <string>

// ダメージ数字のポップアップ
struct DamagePopup
{
	Vector2 position;
	int damage;
	float lifetime;
	float maxLifetime;
	bool isCritical;
	bool isHeal;

	DamagePopup(const Vector2& pos, int dmg, bool crit = false, bool heal = false)
		: position(pos)
		, damage(dmg)
		, lifetime(0.0f)
		, maxLifetime(1.0f)
		, isCritical(crit)
		, isHeal(heal)
	{}

	bool IsExpired() const { return lifetime >= maxLifetime; }
	float GetAlpha() const
	{
		float t = lifetime / maxLifetime;
		if (t < 0.2f) return t / 0.2f;  // フェードイン
		if (t > 0.7f) return 1.0f - (t - 0.7f) / 0.3f;  // フェードアウト
		return 1.0f;
	}
};

// 通知メッセージの種類
enum class NotificationType
{
	ItemPickup,
	ItemUse,
	EquipChange,
	LevelUp,
	QuestUpdate,
	Warning,
	Info
};

// 画面通知
struct Notification
{
	std::string message;
	NotificationType type;
	float lifetime;
	float maxLifetime;

	Notification(const std::string& msg, NotificationType t, float duration = 2.0f)
		: message(msg)
		, type(t)
		, lifetime(0.0f)
		, maxLifetime(duration)
	{}

	bool IsExpired() const { return lifetime >= maxLifetime; }
	float GetAlpha() const
	{
		float t = lifetime / maxLifetime;
		if (t < 0.1f) return t / 0.1f;
		if (t > 0.8f) return 1.0f - (t - 0.8f) / 0.2f;
		return 1.0f;
	}
};

// 画面エフェクト
struct ScreenEffect
{
	enum class Type
	{
		Damage,      // 画面端が赤くなる
		Heal,        // 画面端が緑に光る
		LowHealth,   // 低HP時の警告
		Flash        // 白フラッシュ
	};

	Type type;
	float intensity;
	float lifetime;
	float maxLifetime;

	ScreenEffect(Type t, float inten, float duration)
		: type(t)
		, intensity(inten)
		, lifetime(0.0f)
		, maxLifetime(duration)
	{}

	bool IsExpired() const { return lifetime >= maxLifetime; }
	float GetCurrentIntensity() const
	{
		float t = lifetime / maxLifetime;
		return intensity * (1.0f - t);
	}
};

// フィードバックシステム（シングルトン）
class FeedbackSystem
{
public:
	static FeedbackSystem& GetInstance()
	{
		static FeedbackSystem instance;
		return instance;
	}

	void Update(float deltaTime);
	void Draw(const Vector2& cameraOffset) const;
	void DrawScreenEffects() const;
	void DrawNotifications() const;

	// ダメージポップアップ
	void ShowDamage(const Vector2& worldPos, int damage, bool isCritical = false);
	void ShowHeal(const Vector2& worldPos, int amount);

	// 通知
	void ShowNotification(const std::string& message, NotificationType type);
	void ShowItemPickup(const std::string& itemName, int count = 1);
	void ShowItemUse(const std::string& itemName);
	void ShowEquipChange(const std::string& itemName, bool equipped);
	void ShowWarning(const std::string& message);

	// 画面エフェクト
	void TriggerDamageEffect(float intensity = 0.5f);
	void TriggerHealEffect(float intensity = 0.5f);
	void TriggerLowHealthWarning();

	void Clear();

private:
	FeedbackSystem() = default;
	FeedbackSystem(const FeedbackSystem&) = delete;
	FeedbackSystem& operator=(const FeedbackSystem&) = delete;

	std::vector<DamagePopup> damagePopups_;
	std::vector<Notification> notifications_;
	std::vector<ScreenEffect> screenEffects_;

	static constexpr int MAX_NOTIFICATIONS = 5;
	static constexpr int NOTIFICATION_X = 10;
	static constexpr int NOTIFICATION_Y = 500;
	static constexpr int NOTIFICATION_SPACING = 25;
};
