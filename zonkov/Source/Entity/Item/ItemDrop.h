#pragma once
#include "Utility/Vector2.h"

// マップ上に落ちているアイテム
struct ItemDrop
{
	int itemId;
	int stackCount;
	Vector2 position;
	float pickupRadius;
	bool isPickedUp;

	ItemDrop()
		: itemId(0)
		, stackCount(0)
		, position(0.0f, 0.0f)
		, pickupRadius(20.0f)
		, isPickedUp(false)
	{}

	ItemDrop(int id, int count, const Vector2& pos)
		: itemId(id)
		, stackCount(count)
		, position(pos)
		, pickupRadius(20.0f)
		, isPickedUp(false)
	{}

	bool IsActive() const { return !isPickedUp && itemId != 0; }
};
