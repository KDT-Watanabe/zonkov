#pragma once
#include "Vector2.h"

struct RectF
{
	float x;
	float y;
	float width;
	float height;

	RectF() : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
	RectF(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
	RectF(const Vector2& pos, float width, float height) : x(pos.x), y(pos.y), width(width), height(height) {}

	float Left() const { return x; }
	float Right() const { return x + width; }
	float Top() const { return y; }
	float Bottom() const { return y + height; }
	Vector2 Center() const { return Vector2(x + width * 0.5f, y + height * 0.5f); }

	bool Intersects(const RectF& other) const
	{
		return Left() < other.Right() &&
			Right() > other.Left() &&
			Top() < other.Bottom() &&
			Bottom() > other.Top();
	}

	bool Contains(const Vector2& point) const
	{
		return point.x >= Left() && point.x <= Right() &&
			point.y >= Top() && point.y <= Bottom();
	}

	bool IntersectsCircle(const Vector2& center, float radius) const
	{
		float closestX = center.x;
		float closestY = center.y;

		if (closestX < Left()) closestX = Left();
		if (closestX > Right()) closestX = Right();
		if (closestY < Top()) closestY = Top();
		if (closestY > Bottom()) closestY = Bottom();

		float dx = center.x - closestX;
		float dy = center.y - closestY;
		return (dx * dx + dy * dy) <= (radius * radius);
	}
};
