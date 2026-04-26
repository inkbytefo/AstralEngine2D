#pragma once
#include <cmath>
// Oyun dünyasındaki 2 boyutlu koordinat ve hız vektörlerini temsil eder.
// Operatör aşırı yüklemeleri ile vektörel matematik işlemlerini kolaylaştırır.
struct Vec2
{
	float x { 0.0f };
	float y{ 0.0f };
	
	Vec2() = default;
	Vec2(float x, float y) : x(x), y(y) {}

	// Vektörlerin toplanması, konumların veya hızların birleştirilmesi için kullanılır.
	Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
	// İki nokta arasındaki farkı (yönü) bulmak için kullanılır.
	Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
	// Vektörün büyüklüğünü (hız gibi) ölçeklendirmek için kullanılır.
	Vec2 operator*(float scalar)      const { return { x * scalar,  y * scalar }; }
	Vec2 operator/(float scalar)      const { return { x / scalar,  y / scalar }; }

	// Mevcut konuma hız eklemek gibi yerinde güncellemeler için performans sağlar.
	Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
	Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
	Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
	Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

	float lengthSq() const { return (x * x) + (y * y); }
	float length() const { return std::sqrt(lengthSq()); }
	float distSq(const Vec2& other) const
	{
		return (*this - other).lengthSq();
	}
	float normalize()
	{
		float len = length();
		if (len > 0)
		{
			x /= len;
			y /= len;
		}
	}

};
