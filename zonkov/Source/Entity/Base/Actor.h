#pragma once
#include "Utility/Vector2.h"

// 戦闘可能なエンティティの基底クラス
class Actor
{
public:
	Actor();
	virtual ~Actor() = default;

	virtual void Update(float deltaTime) = 0;
	virtual void Draw(const Vector2& cameraOffset) const = 0;

	// 位置・当たり判定
	const Vector2& GetPosition() const { return position_; }
	void SetPosition(const Vector2& pos) { position_ = pos; }
	float GetRadius() const { return radius_; }
	void SetRadius(float r) { radius_ = r; }

	// HP
	int GetHP() const { return hp_; }
	int GetMaxHP() const { return maxHP_; }
	void SetHP(int hp) { hp_ = hp; }
	void SetMaxHP(int maxHP) { maxHP_ = maxHP; hp_ = maxHP; }
	void TakeDamage(int amount);
	int Heal(int amount);  // 実際に回復した量を返す
	bool IsAlive() const { return hp_ > 0; }

protected:
	Vector2 position_;
	float radius_;
	int hp_;
	int maxHP_;
};
