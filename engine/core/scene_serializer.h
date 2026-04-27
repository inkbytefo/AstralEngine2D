#pragma once

#include <string>
#include <memory>
#include <map>
#include "core/json.hpp"

namespace Astral {
    class EntityManager;
    class Entity;
}

/**
 * @brief Scene Serializer - Sahneleri JSON formatında kaydetme ve yükleme
 * 
 * Tüm entity'leri ve component'larını (has == true olanları) .astral dosyasına kaydeder.
 * Deserialization sırasında ID remapping yaparak parent-child ilişkilerini korur.
 */
class SceneSerializer {
public:
    /**
     * @brief Sahneyi .astral dosyasına kaydet
     * @param filepath Kaydedilecek dosya yolu (örn: "scenes/level1.astral")
     * @param entityManager Kaydedilecek entity'leri içeren EntityManager
     * @return Başarılı ise true
     */
    static bool serialize(const std::string& filepath, 
                         Astral::EntityManager& entityManager);
    
    /**
     * @brief .astral dosyasından sahneyi yükle
     * @param filepath Yüklenecek dosya yolu
     * @param entityManager Yüklenen entity'lerin ekleneceği EntityManager
     * @return Başarılı ise true
     */
    static bool deserialize(const std::string& filepath, 
                           Astral::EntityManager& entityManager);
    
    /**
     * @brief Sahnenin snapshot'ını (JSON string) al
     * Play moduna geçerken kullanılır
     * @param entityManager Snapshot alınacak EntityManager
     * @return JSON string formatında snapshot
     */
    static std::string createSnapshot(Astral::EntityManager& entityManager);
    
    /**
     * @brief Snapshot'tan sahneyi restore et
     * Stop moduna geçerken kullanılır
     * @param jsonString Snapshot JSON string'i
     * @param entityManager Restore edilecek EntityManager
     * @return Başarılı ise true
     */
    static bool restoreFromSnapshot(const std::string& jsonString,
                                   Astral::EntityManager& entityManager);

private:
    // ID Mapping - Eski ID'leri yeni ID'lere eşleştir
    using IDMap = std::map<uint32_t, uint32_t>;

    /**
     * @brief Tek bir entity'yi JSON'a dönüştür
     */
    static nlohmann::json serializeEntity(const std::shared_ptr<Astral::Entity>& entity);
    
    /**
     * @brief JSON'dan entity oluştur
     */
    static std::shared_ptr<Astral::Entity> deserializeEntity(
        const nlohmann::json& json, 
        Astral::EntityManager& entityManager,
        IDMap& idMap);
    
    /**
     * @brief Parent-child ilişkilerini ID mapping'i kullanarak kur
     */
    static void restoreHierarchy(const nlohmann::json& entitiesJson,
                                Astral::EntityManager& entityManager,
                                const IDMap& idMap);
};
