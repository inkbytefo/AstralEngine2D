#pragma once
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "ecs/entity.h"

namespace Astral {

// Varlık listeleri için kısa adlar, kodun okunabilirliğini artırır.
using EntityVec = std::vector<std::shared_ptr<Entity>>;
using EntityMap = std::map<std::string, EntityVec>;

// Varlıkların yaşam döngüsünü (ekleme, silme, güncelleme) yöneten merkezi sistem.
// Liste üzerinde dönerken ekleme/silme yapıldığında oluşabilecek hataları (iterator invalidation) önlemek için gecikmeli işlem yapar.
class EntityManager
{
public:
	// Silinmiş varlıkları temizler ve yeni bekleyen varlıkları ana listeye dahil eder.
	void update();
	// Tüm varlıkları temizler ve ID sayacını sıfırlar.
	void clear();
	// Yeni bir varlık oluşturur ve bir sonraki karede sisteme dahil edilmek üzere beklemeye alır.
	std::shared_ptr<Entity>		addEntity(const std::string& tag);
	
	// Tüm aktif varlıklara erişim sağlar.
	EntityVec&					getEntities();
	// Henüz activate edilmemiş, sonraki update'te görünür olacak varlıklar.
	const EntityVec&			getPendingEntities() const;
	// Belirli bir etikete ait varlık listesi varsa döndürür, yoksa nullptr.
	const EntityVec*            tryGetEntities(const std::string& tag) const;
	// Belirli bir etikete sahip varlıklara (örn: sadece mermiler) hızlıca erişmek için kullanılır.
	EntityVec&					getEntities(const std::string& tag);
    
    // Component bazlı filtreleme (ECS view pattern) - Optimized version
    // Örnek: auto entities = entityManager.view<CTransform, CCamera>();
    template <typename... T>
    EntityVec view()
    {
        // Pre-allocate with estimated capacity to reduce reallocations
        EntityVec result;
        result.reserve(m_entities.size() / 4); // Estimate 25% will match

        // Cache-friendly iteration - process entities in contiguous memory
        for (auto& entity : m_entities)
        {
            // Early exit for inactive entities (common case)
            if (!entity->isActive()) continue;

            // Use fold expression for compile-time optimization
            if constexpr (sizeof...(T) == 0) {
                // No components specified - return all active entities
                result.push_back(entity);
            } else {
                // Check if entity has all required components
                bool hasAllComponents = (entity->has<T>() && ...);
                if (hasAllComponents) {
                    result.push_back(entity);
                }
            }
        }

        return result;
    }

    template <typename... T, typename Func>
    void each(Func&& func)
    {
        for (auto& entity : m_entities)
        {
            if (!entity->isActive()) continue;

            if constexpr (sizeof...(T) == 0) {
                func(entity);
            } else {
                if ((entity->has<T>() && ...)) {
                    func(entity);
                }
            }
        }
    }



private:
	EntityVec		m_entities;    // Tüm varlıkların ana listesi
	EntityVec		m_toAdd;       // Mevcut kare bittikten sonra eklenecekler (eşzamanlılık güvenliği)
	EntityMap		m_entityMap;   // Etiketlere göre hızlı erişim sağlayan harita
	uint32_t		m_totalEntites{ 0 }; // Benzersiz ID üretimi için sayaç

};

} // namespace Astral


using EntityManager = Astral::EntityManager;
using Entity = Astral::Entity;
using EntityVec = Astral::EntityVec;
