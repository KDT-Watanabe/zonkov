#include "Actor.h"

Actor::Actor()
	: position_(0.0f, 0.0f)
	, radius_(12.0f)
	, hp_(100)
	, maxHP_(100)
{
}

void Actor::TakeDamage(int amount)
{
	hp_ -= amount;
	if (hp_ < 0)
	{
		hp_ = 0;
	}
}

int Actor::Heal(int amount)
{
	int oldHP = hp_;
	hp_ += amount;
	if (hp_ > maxHP_)
	{
		hp_ = maxHP_;
	}
	return hp_ - oldHP;  // 実際に回復した量
}
