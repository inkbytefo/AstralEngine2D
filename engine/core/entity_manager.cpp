#include "core/entity_manager.h"
#include <algorithm>

namespace Astral {

void EntityManager::update()
{
	// Yeni varlıkları ana listelere ekleyerek oyun dünyasına dahil eder.
	// Bu adım, ekleme işleminin döngüler sırasında çakışmasını önlemek için ayrı yapılır.
	for (auto& e : m_toAdd)
	{
		m_entities.push_back(e);
		m_entityMap[e->tag()].push_back(e);
	}
	m_toAdd.clear();

	// Pasif (destroy çağrılmış) varlıkları bellekten ve listelerden temizleyen yardımcı fonksiyon.
	// shared_ptr kullanımı sayesinde referans kalmadığında bellek otomatik iade edilir.
	auto removeDeadEntites = [](EntityVec& vec)
		{
			vec.erase(
				std::remove_if(vec.begin(), vec.end(),
					[](const std::shared_ptr<Entity>& e) { return !e->isActive(); }),
				vec.end()
			);
		};

	// Hem genel listeyi hem de kategori bazlı haritayı güncel tutar.
	removeDeadEntites(m_entities);
	for (auto& [tag, vec] : m_entityMap)
		removeDeadEntites(vec);
}

void EntityManager::clear()
{
	m_entities.clear();
	m_toAdd.clear();
	m_entityMap.clear();
	m_totalEntites = 0;
}

std::shared_ptr<Entity> EntityManager::addEntity(const std::string& tag)
{
	// Yeni varlığı oluşturur ancak güvenli bir şekilde eklenmesi için m_toAdd listesine koyar.
	auto entity = std::shared_ptr<Entity>(new Entity(++m_totalEntites, tag));
	m_toAdd.push_back(entity);
	return entity;
}

EntityVec& EntityManager::getEntities()
{
	return m_entities;
}

const EntityVec& EntityManager::getPendingEntities() const
{
	return m_toAdd;
}

const EntityVec* EntityManager::tryGetEntities(const std::string& tag) const
{
	auto it = m_entityMap.find(tag);
	return (it != m_entityMap.end()) ? &it->second : nullptr;
}

EntityVec& EntityManager::getEntities(const std::string& tag)
{
	// İlgili etikete ait varlık listesini döndürür.
	return m_entityMap[tag];
}

} // namespace Astral
