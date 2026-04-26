#pragma once
#include "math/vec2.h"
#include <stdint.h>

// Varlıkların dünyadaki fiziksel varlığını (konum ve hız) temsil eder.
// Veri odaklı tasarım (ECS) prensibi gereği mantıktan ayrılmıştır.
struct CTransform
{
	Vec2 pos;
	Vec2 velocity;

	CTransform() = default;
	CTransform(const Vec2& pos, const Vec2& velocity)
		: pos(pos), velocity(velocity) {}
};

// Varlıkların ekranda nasıl görüneceğini (boyut ve renk) tanımlar.
// Çizim sistemi bu verileri kullanarak SDL_Render komutlarını oluşturur.
struct CShape
{
	float width{ 0.0f };
	float height{ 0.0f };

	// RGBA renk değerleri, her biri 0-255 arası tamsayıdır.
	uint8_t r{ 255 }, g{ 255 }, b{ 255 }, a{ 255 };

	CShape() = default;
	CShape(float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: width(w), height(h), r(r), g(g), b(b), a(a) {}

};

inline bool checkCollision(const CTransform& a, const CShape& as,
	const CTransform& b, const CShape& bs)
{
	// buraya AABB formülünü uygula
	float aLeft = a.pos.x;
	float aRight = a.pos.x + as.width;
	float aTop = a.pos.y;
	float aBottom = a.pos.y + as.height;

	float bLeft = b.pos.x;
	float bRight = b.pos.x + bs.width;
	float bTop = b.pos.y;
	float bBottom = b.pos.y + bs.height;
	
	return aLeft < bRight && aRight > bLeft && aTop < bBottom && aBottom > bTop;
}