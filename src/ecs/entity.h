#pragma once
#include <memory>
#include <string>
#include "components.h"

// Oyundaki her türlü nesnenin (oyuncu, düşman, mermi) temelini oluşturur.
// Sadece veri (bileşenler) ve kimlik bilgilerini barındıran hafif bir sınıftır.
class Entity
{
	// Bellek yönetimini ve varlık oluşturma sürecini kontrol altında tutmak için EntityManager'a yetki verilir.
	friend class EntityManager;

public:
	CTransform	transform;
	CShape		shape;

	// Varlığın hala aktif olup olmadığını kontrol eder, silinme aşamasındaki varlıkları filtrelemek için kullanılır.
	bool isActive() const	{ return m_active; }
	// Varlığı bir sonraki karede temizlenmek üzere işaretler.
	void destroy()			{ m_active = false; }

	// Her varlığa atanan benzersiz kimlik, hata ayıklama ve takip için kritiktir.
	uint32_t id() const		{ return m_id; }
	// Gruplandırma (örn: "bullet", "enemy") yaparak toplu işlemler yapmayı sağlar.
	std::string tag() const { return m_tag; }

private:
	// Constructor private tutularak varlıkların rastgele değil, sadece yönetici üzerinden oluşturulması garanti edilir.
	Entity(uint32_t id, const std::string& tag)
		: m_id(id), m_tag(tag) {}

	uint32_t	m_id{ 0 };
	std::string m_tag;
	bool		m_active{ true };
};