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
	CSprite,
	CMesh,
	CCamera,
    CLight
>;

// Oyun içerisindeki tüm dinamik veya statik objelerin (Karakter, Silah, Kamera, Işık vb.) soyut temelini oluşturur.
// Data-Oriented Design (DOD) mimarisine uygun olarak, nesne tabanlı kalıtım yerine kompozisyon kullanır.
// Bu sınıf doğrudan hiçbir oyun mantığı barındırmaz, sadece kimlik (ID), etiket (Tag) ve kendisine ait Bileşenleri (Component) yönetir.
class Entity
{
	// Yaşam döngüsü ve bellek tahsisi (Memory Allocation) sadece EntityManager tarafından yönetilmelidir.
	friend class EntityManager;

public:
	// Nesnenin sahnede aktif olup olmadığını döndürür. Silinmek üzere işaretlenmiş nesneler false döner.
	bool isActive() const	{ return m_active; }
	
    // Nesneyi bir sonraki karede (frame) hafızadan kalıcı olarak silinmek üzere işaretler.
	void destroy()			{ m_active = false; }

	// Nesneye ait sistem bazında eşsiz (unique) kimlik numarası.
	uint32_t id() const		{ return m_id; }
	
    // Nesneleri filtrelemek ve gruplamak için kullanılan tanımlayıcı etiket (Örn: "enemy", "bullet").
	std::string tag() const { return m_tag; }

	// İstenilen tipteki bileşene (Component) doğrudan referans döndürür.
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

	// Public component access (ECS pattern için)
	CTransform& cTransform = get<CTransform>();
	CShape& cShape = get<CShape>();
	CBBox& cBBox = get<CBBox>();
	CInput& cInput = get<CInput>();
	CLifeSpan& cLifeSpan = get<CLifeSpan>();
	CText& cText = get<CText>();
	CSprite& cSprite = get<CSprite>();
	CMesh& cMesh = get<CMesh>();
	CCamera& cCamera = get<CCamera>();
    CLight& cLight = get<CLight>();

private:
	// Constructor private tutularak varlıkların rastgele değil, sadece yönetici üzerinden oluşturulması garanti edilir.
	Entity(uint32_t id, const std::string& tag)
		: m_id(id), m_tag(tag) {}

	uint32_t	m_id{ 0 };
	std::string m_tag;
	bool		m_active{ true };
	ComponentTuple m_components; // Varlığın sahip olduğu bileşenlerin saklandığı tuple
};