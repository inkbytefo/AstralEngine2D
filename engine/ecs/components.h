#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <string>
#include <SDL3_ttf/SDL_ttf.h>

#include <vector>
#include <memory>
class Entity;

// Entity (Varlık) nesnelerinin dünya uzayındaki (World Space) pozisyon, hız, ölçek ve rotasyon verilerini barındırır.
// ECS (Entity-Component-System) ve DOD (Data-Oriented Design) mimarisi gereği, bu yapı içerisinde herhangi bir metot barındırılmaz.
// Sadece veriyi yapısal olarak saklamakla yükümlüdür.
struct CTransform
{
	glm::vec3 pos{ 0.0f, 0.0f, 0.0f };      // Yerel (Local) 3D pozisyon vektörü
	glm::vec3 velocity{ 0.0f, 0.0f, 0.0f }; // 3D Hız (Velocity) vektörü
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };    // Yerel boyut/ölçek çarpanı
	glm::vec3 rotation{ 0.0f, 0.0f, 0.0f }; // Yerel dönme açıları (Euler cinsinden)

	// Scene Graph (Transform Hiyerarşisi) Yönetimi
	std::weak_ptr<Entity> parent;                  // Ebeveyn nesneye olan referans (Döngüsel bellek sızıntısını önlemek için weak_ptr kullanılmıştır)
	std::vector<std::shared_ptr<Entity>> children; // Alt (çocuk) nesnelerin listesi

	// Önceden hesaplanmış, güncel Dünya Koordinat Matrisi (World Transform Matrix)
	glm::mat4 globalMatrix{ 1.0f };

	bool has{ false };

	CTransform() = default;
	CTransform(const glm::vec3& p, const glm::vec3& v)
		: pos(p), velocity(v) {}
	CTransform(const glm::vec2& p, const glm::vec2& v)
		: pos(glm::vec3(p, 0.0f)), velocity(glm::vec3(v, 0.0f)) {}
};

// 2D çizim işlemleri için (Debug veya basit UI elemanları) kullanılan temel fiziksel ölçüleri ve RGBA renk bilgisini tanımlar.
struct CShape
{
	float width{ 0.0f };
	float height{ 0.0f };

	// RGBA renk kanalları (0-255 aralığında)
	uint8_t r{ 255 }, g{ 255 }, b{ 255 }, a{ 255 };
	bool has{ false };

	CShape() = default;
	CShape(float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: width(w), height(h), r(r), g(g), b(b), a(a) {}

};

struct CBBox
{
	bool has{ false };
	float width{ 0.0f }, height{ 0.0f };

	CBBox() = default;
	CBBox(float w, float h) :width(w), height(h) {}
};

struct CInput
{
	bool has{ false };
	bool up{ false }, down{ false }, left{ false }, right{ false };
};

struct CSprite
{
	bool has{ false };
	SDL_GPUTexture* texture{ nullptr };
	SDL_FRect srcRect{ 0.0f, 0.0f, 0.0f, 0.0f }; // Texture'dan hangi bölümü çizeceğimizi tanımlar
	float angle{ 0.0f }; // Sprite'ın döndürülme açısı

	CSprite() = default;
	CSprite(SDL_GPUTexture* tex) : texture(tex) 
	{
		if (tex)
		{
			// SDL_GPUTexture boyutu farkli alinir
			/*
			float w, h;
			if (SDL_GetTextureSize(tex, &w, &h))
			{
				srcRect = { 0.0f, 0.0f, w, h };
			}
			*/
		}
	}
};

// 3D Mesh Component - GPU'da yaşayan mesh referansı
struct CMesh
{
	bool has{ false };
	std::string meshName; // AssetManager'daki mesh ismi
	std::string materialName; // Pipeline/Material ismi (opsiyonel)

	CMesh() = default;
	CMesh(const std::string& mesh, const std::string& material = "default")
		: meshName(mesh), materialName(material) {}
};

// Kamera Bileşeni - Render System bu bileşeni arayıp MVP hesaplamalarında kullanacak
struct CCamera
{
	bool has{ false };
	bool isActive{ true }; // Birden fazla kamera varsa hangisi aktif?
	glm::mat4 view{ 1.0f };
	glm::mat4 projection{ 1.0f };

	CCamera() = default;
	CCamera(const glm::mat4& v, const glm::mat4& p)
		: view(v), projection(p) {}
};

// Free Look - Kamera kontrol verileri
struct CFreeLook
{
    bool has{ false };
    float yaw{ -90.0f };
    float pitch{ 0.0f };
    float speed{ 10.0f };
    float sensitivity{ 0.1f };
    
    // Mouse durumu
    bool isRightMouseDown{ false };
    float lastX{ 0.0f }, lastY{ 0.0f };

    CFreeLook() = default;
    CFreeLook(float s, float sens) : speed(s), sensitivity(sens) {}
};

// Işık bileşeni (PBR için)
struct CLight
{
	bool has{ false };
	glm::vec3 color{ 1.0f, 1.0f, 1.0f };
	float intensity{ 1.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f }; // Yönlü ışık için

	CLight() = default;
	CLight(const glm::vec3& c, float i, const glm::vec3& dir)
		: color(c), intensity(i), direction(dir) {}
};

struct CLifeSpan
{
	bool has{ false };
	float remaining{ 0.0f };
	float total{ 0.0f };

	CLifeSpan() = default;
	CLifeSpan(float totalTime) : total(totalTime), remaining(totalTime) {}
};

struct CText
{
	bool has{ false };
	std::string text;
	TTF_Font* font{ nullptr };
	SDL_Color color{ 255, 255, 255, 255 };

	// Cache için eklenenler
	SDL_GPUTexture* texture{ nullptr }; 
	float width{ 0 }, height{ 0 };
	bool needsUpdate{ true }; // Metin değiştiğinde true yapılacak

	CText() = default;
	CText(const std::string& t, TTF_Font* f, const SDL_Color& c)
		: text(t), font(f), color(c), needsUpdate(true), texture(nullptr) {
	}
	~CText() {}

	void cleanup()
	{
		if (texture)
		{
			// SDL_ReleaseGPUTexture(m_gpuDevice, texture); // Device referansi lazim
			texture = nullptr;
		}
	}

	// Metni güncellemek için yardımcı fonksiyon
    void setText(const std::string& newText) {
        if (text != newText) {
            text = newText;
            needsUpdate = true;
        }
    }
};


inline bool checkCollision(const CTransform& a, const CShape& as,
	const CTransform& b, const CShape& bs)
{
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

