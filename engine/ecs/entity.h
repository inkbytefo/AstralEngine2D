#pragma once
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include "components.h"

using ComponentTuple = std::tuple<
	CTransform,
	CShape,
	CBBox,
	CInput,
	CLifeSpan,
	CText,
	CSprite
>;

// Oyundaki her türlü nesnenin (oyuncu, düşman, mermi) temelini oluşturur.
// Sadece veri (bileşenler) ve kimlik bilgilerini barındıran hafif bir sınıftır.
class Entity
{
	// Bellek yönetimini ve varlık oluşturma sürecini kontrol altında tutmak için EntityManager'a yetki verilir.
	friend class EntityManager;

public:
	// Varlığın hala aktif olup olmadığını kontrol eder, silinme aşamasındaki varlıkları filtrelemek için kullanılır.
	bool isActive() const	{ return m_active; }
	// Varlığı bir sonraki karede temizlenmek üzere işaretler.
	void destroy()			{ m_active = false; }

	// Her varlığa atanan benzersiz kimlik, hata ayıklama ve takip için kritiktir.
	uint32_t id() const		{ return m_id; }
	// Gruplandırma (örn: "bullet", "enemy") yaparak toplu işlemler yapmayı sağlar.
	std::string tag() const { return m_tag; }

	template <typename T>
	T& get()
	{
		return std::get<T>(m_components);
	}

	template <typename T>
	bool has() 
	{
		return std::get<T>(m_components).has;
	}

	template <typename T, typename... Args>
	T& add(Args&&... mArgs)
	{
		auto& component = get<T>();
		component = T(std::forward<Args>(mArgs)...);
		component.has = true;
		return component;
	}

	template <typename T>
	void remove()
	{
		get<T>().has = false;
	}
private:
	// Constructor private tutularak varlıkların rastgele değil, sadece yönetici üzerinden oluşturulması garanti edilir.
	Entity(uint32_t id, const std::string& tag)
		: m_id(id), m_tag(tag) {}

	uint32_t	m_id{ 0 };
	std::string m_tag;
	bool		m_active{ true };
	ComponentTuple m_components; // Varlığın sahip olduğu bileşenlerin saklandığı tuple
};