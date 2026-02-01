#pragma once
#include <cmath>

struct Vector2
{
	float x;
	float y;

	Vector2() : x(0.0f), y(0.0f) {}
	Vector2(float x, float y) : x(x), y(y) {}

	Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
	Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
	Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
	Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }
	Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
	Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
	Vector2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
	bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
	bool operator!=(const Vector2& other) const { return !(*this == other); }

	float LengthSquared() const { return x * x + y * y; }
	float GetLength() const { return std::sqrtf(LengthSquared()); }

	Vector2 Normalized() const
	{
		float len = GetLength();
		if (len == 0.0f) return Vector2(0.0f, 0.0f);
		return Vector2(x / len, y / len);
	}

	float Distance(const Vector2& other) const { return (*this - other).GetLength(); }
	float Dot(const Vector2& other) const { return x * other.x + y * other.y; }

	static const Vector2 Zero;
	static const Vector2 One;
	static const Vector2 Up;
	static const Vector2 Down;
	static const Vector2 Left;
	static const Vector2 Right;
};
