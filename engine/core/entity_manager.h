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
	// Belirli bir etikete sahip varlıklara (örn: sadece mermiler) hızlıca erişmek için kullanılır.
	EntityVec&					getEntities(const std::string& tag);
    
    // Component bazlı filtreleme (ECS view pattern)
    // Örnek: auto entities = entityManager.view<CTransform, CCamera>();
    template <typename... T>
    EntityVec view()
    {
        EntityVec result;
        for (auto& entity : m_entities)
        {
            if (entity->isActive() && (entity->has<T>() && ...))
            {
                result.push_back(entity);
            }
        }
        return result;
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