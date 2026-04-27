#include "core/scene_serializer.h"
#include "core/entity_manager.h"
#include "ecs/entity.h"
#include "ecs/components.h"
#include <fstream>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

using json = nlohmann::json;

// Helper: glm::vec3 -> JSON array
static json vec3ToJson(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

// Helper: JSON array -> glm::vec3
static glm::vec3 jsonToVec3(const json& j) {
    if (j.is_array() && j.size() == 3) {
        return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
    }
    return glm::vec3(0.0f);
}

// Helper: glm::vec4 -> JSON array
static json vec4ToJson(const glm::vec4& v) {
    return json::array({v.x, v.y, v.z, v.w});
}

// Helper: JSON array -> glm::vec4
static glm::vec4 jsonToVec4(const json& j) {
    if (j.is_array() && j.size() == 4) {
        return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
    }
    return glm::vec4(0.0f);
}

// Helper: glm::vec2 -> JSON array
static json vec2ToJson(const glm::vec2& v) {
    return json::array({v.x, v.y});
}

// Helper: JSON array -> glm::vec2
static glm::vec2 jsonToVec2(const json& j) {
    if (j.is_array() && j.size() == 2) {
        return glm::vec2(j[0].get<float>(), j[1].get<float>());
    }
    return glm::vec2(0.0f);
}

// ============================================================================
// COMPONENT SERIALIZATION HELPERS
// ============================================================================

static json serializeCTransform(const CTransform& comp) {
    json j;
    j["pos"] = vec3ToJson(comp.pos);
    j["velocity"] = vec3ToJson(comp.velocity);
    j["scale"] = vec3ToJson(comp.scale);
    j["rotation"] = vec3ToJson(comp.rotation);
    j["parentId"] = nullptr;  // Parent ID'si hiyerarşi restore sırasında ayarlanacak
    j["childrenIds"] = json::array();  // Children ID'leri hiyerarşi restore sırasında ayarlanacak
    return j;
}

static CTransform deserializeCTransform(const json& j) {
    CTransform comp;
    if (j.contains("pos")) comp.pos = jsonToVec3(j["pos"]);
    if (j.contains("velocity")) comp.velocity = jsonToVec3(j["velocity"]);
    if (j.contains("scale")) comp.scale = jsonToVec3(j["scale"]);
    if (j.contains("rotation")) comp.rotation = jsonToVec3(j["rotation"]);
    comp.has = true;
    return comp;
}

static json serializeCShape(const CShape& comp) {
    json j;
    j["width"] = comp.width;
    j["height"] = comp.height;
    j["r"] = comp.r;
    j["g"] = comp.g;
    j["b"] = comp.b;
    j["a"] = comp.a;
    return j;
}

static CShape deserializeCShape(const json& j) {
    CShape comp;
    if (j.contains("width")) comp.width = j["width"].get<float>();
    if (j.contains("height")) comp.height = j["height"].get<float>();
    if (j.contains("r")) comp.r = j["r"].get<uint8_t>();
    if (j.contains("g")) comp.g = j["g"].get<uint8_t>();
    if (j.contains("b")) comp.b = j["b"].get<uint8_t>();
    if (j.contains("a")) comp.a = j["a"].get<uint8_t>();
    comp.has = true;
    return comp;
}

static json serializeCBBox(const CBBox& comp) {
    json j;
    j["width"] = comp.width;
    j["height"] = comp.height;
    return j;
}

static CBBox deserializeCBBox(const json& j) {
    CBBox comp;
    if (j.contains("width")) comp.width = j["width"].get<float>();
    if (j.contains("height")) comp.height = j["height"].get<float>();
    comp.has = true;
    return comp;
}

static json serializeCInput(const CInput& comp) {
    json j;
    j["up"] = comp.up;
    j["down"] = comp.down;
    j["left"] = comp.left;
    j["right"] = comp.right;
    return j;
}

static CInput deserializeCInput(const json& j) {
    CInput comp;
    if (j.contains("up")) comp.up = j["up"].get<bool>();
    if (j.contains("down")) comp.down = j["down"].get<bool>();
    if (j.contains("left")) comp.left = j["left"].get<bool>();
    if (j.contains("right")) comp.right = j["right"].get<bool>();
    comp.has = true;
    return comp;
}

static json serializeCLifeSpan(const CLifeSpan& comp) {
    json j;
    j["remaining"] = comp.remaining;
    j["total"] = comp.total;
    return j;
}

static CLifeSpan deserializeCLifeSpan(const json& j) {
    CLifeSpan comp;
    if (j.contains("remaining")) comp.remaining = j["remaining"].get<float>();
    if (j.contains("total")) comp.total = j["total"].get<float>();
    comp.has = true;
    return comp;
}

static json serializeCText(const CText& comp) {
    json j;
    j["text"] = comp.text;
    j["color"] = json::object({
        {"r", comp.color.r},
        {"g", comp.color.g},
        {"b", comp.color.b},
        {"a", comp.color.a}
    });
    // Font pointer ve texture saklanmaz - runtime'da yeniden yüklenir
    return j;
}

static CText deserializeCText(const json& j) {
    CText comp;
    if (j.contains("text")) comp.text = j["text"].get<std::string>();
    if (j.contains("color")) {
        auto& color = j["color"];
        comp.color.r = color.value("r", 255);
        comp.color.g = color.value("g", 255);
        comp.color.b = color.value("b", 255);
        comp.color.a = color.value("a", 255);
    }
    comp.has = true;
    return comp;
}

static json serializeCSprite(const CSprite& comp) {
    json j;
    j["angle"] = comp.angle;
    j["srcRect"] = json::object({
        {"x", comp.srcRect.x},
        {"y", comp.srcRect.y},
        {"w", comp.srcRect.w},
        {"h", comp.srcRect.h}
    });
    // Texture pointer saklanmaz - runtime'da yeniden yüklenir
    return j;
}

static CSprite deserializeCSprite(const json& j) {
    CSprite comp;
    if (j.contains("angle")) comp.angle = j["angle"].get<float>();
    if (j.contains("srcRect")) {
        auto& rect = j["srcRect"];
        comp.srcRect.x = rect.value("x", 0.0f);
        comp.srcRect.y = rect.value("y", 0.0f);
        comp.srcRect.w = rect.value("w", 0.0f);
        comp.srcRect.h = rect.value("h", 0.0f);
    }
    comp.has = true;
    return comp;
}

static json serializeCMesh(const CMesh& comp) {
    json j;
    j["meshName"] = comp.meshName;
    j["materialName"] = comp.materialName;
    return j;
}

static CMesh deserializeCMesh(const json& j) {
    CMesh comp;
    if (j.contains("meshName")) comp.meshName = j["meshName"].get<std::string>();
    if (j.contains("materialName")) comp.materialName = j["materialName"].get<std::string>();
    comp.has = true;
    return comp;
}

static json serializeCCamera(const CCamera& comp) {
    json j;
    j["isActive"] = comp.isActive;
    j["aspectRatio"] = comp.aspectRatio;
    // View ve projection matrisleri saklanmaz - runtime'da hesaplanır
    return j;
}

static CCamera deserializeCCamera(const json& j) {
    CCamera comp;
    if (j.contains("isActive")) comp.isActive = j["isActive"].get<bool>();
    if (j.contains("aspectRatio")) comp.aspectRatio = j["aspectRatio"].get<float>();
    comp.has = true;
    return comp;
}

static json serializeCLight(const CLight& comp) {
    json j;
    j["type"] = static_cast<int32_t>(comp.type);
    j["color"] = vec3ToJson(comp.color);
    j["intensity"] = comp.intensity;
    j["direction"] = vec3ToJson(comp.direction);
    j["range"] = comp.range;
    j["innerCutoff"] = comp.innerCutoff;
    j["outerCutoff"] = comp.outerCutoff;
    return j;
}

static CLight deserializeCLight(const json& j) {
    CLight comp;
    if (j.contains("type")) comp.type = static_cast<LightType>(j["type"].get<int32_t>());
    if (j.contains("color")) comp.color = jsonToVec3(j["color"]);
    if (j.contains("intensity")) comp.intensity = j["intensity"].get<float>();
    if (j.contains("direction")) comp.direction = jsonToVec3(j["direction"]);
    if (j.contains("range")) comp.range = j["range"].get<float>();
    if (j.contains("innerCutoff")) comp.innerCutoff = j["innerCutoff"].get<float>();
    if (j.contains("outerCutoff")) comp.outerCutoff = j["outerCutoff"].get<float>();
    comp.has = true;
    return comp;
}

static json serializeCFreeLook(const CFreeLook& comp) {
    json j;
    j["yaw"] = comp.yaw;
    j["pitch"] = comp.pitch;
    j["speed"] = comp.speed;
    j["sensitivity"] = comp.sensitivity;
    return j;
}

static CFreeLook deserializeCFreeLook(const json& j) {
    CFreeLook comp;
    if (j.contains("yaw")) comp.yaw = j["yaw"].get<float>();
    if (j.contains("pitch")) comp.pitch = j["pitch"].get<float>();
    if (j.contains("speed")) comp.speed = j["speed"].get<float>();
    if (j.contains("sensitivity")) comp.sensitivity = j["sensitivity"].get<float>();
    comp.has = true;
    return comp;
}

static json serializeCTrait(const CTrait& comp) {
    json j;
    json traits = json::array();
    for (const auto& trait : comp.traits) {
        json t;
        t["type"] = trait->getName();
        json props;
        trait->serialize(props);
        t["properties"] = props;
        traits.push_back(t);
    }
    j["traits"] = traits;
    return j;
}

static CTrait deserializeCTrait(const json& j) {
    CTrait comp;
    if (j.contains("traits") && j["traits"].is_array()) {
        for (const auto& tJson : j["traits"]) {
            std::string type = tJson.value("type", "");
            // Note: TraitFactory should be used here.
        }
    }
    comp.has = true;
    return comp;
}

// ============================================================================
// MAIN SERIALIZATION FUNCTIONS
// ============================================================================

json SceneSerializer::serializeEntity(const std::shared_ptr<Astral::Entity>& entity) {
    json entityJson;
    entityJson["id"] = entity->id();
    entityJson["tag"] = entity->tag();
    entityJson["active"] = entity->isActive();
    
    json components;
    
    // CTransform
    if (entity->has<CTransform>()) {
        auto& transform = entity->get<CTransform>();
        json tJson = serializeCTransform(transform);
        
        // Parent ID'sini kaydet
        if (!transform.parent.expired()) {
            tJson["parentId"] = transform.parent.lock()->id();
        }
        
        // Children ID'lerini kaydet
        for (auto& child : transform.children) {
            tJson["childrenIds"].push_back(child->id());
        }
        
        components["CTransform"] = tJson;
    }
    
    // CShape
    if (entity->has<CShape>()) {
        components["CShape"] = serializeCShape(entity->get<CShape>());
    }
    
    // CBBox
    if (entity->has<CBBox>()) {
        components["CBBox"] = serializeCBBox(entity->get<CBBox>());
    }
    
    // CInput
    if (entity->has<CInput>()) {
        components["CInput"] = serializeCInput(entity->get<CInput>());
    }
    
    // CLifeSpan
    if (entity->has<CLifeSpan>()) {
        components["CLifeSpan"] = serializeCLifeSpan(entity->get<CLifeSpan>());
    }
    
    // CText
    if (entity->has<CText>()) {
        components["CText"] = serializeCText(entity->get<CText>());
    }
    
    // CSprite
    if (entity->has<CSprite>()) {
        components["CSprite"] = serializeCSprite(entity->get<CSprite>());
    }
    
    // CMesh
    if (entity->has<CMesh>()) {
        components["CMesh"] = serializeCMesh(entity->get<CMesh>());
    }
    
    // CCamera
    if (entity->has<CCamera>()) {
        components["CCamera"] = serializeCCamera(entity->get<CCamera>());
    }
    
    // CLight
    if (entity->has<CLight>()) {
        components["CLight"] = serializeCLight(entity->get<CLight>());
    }
    
    // CFreeLook
    if (entity->has<CFreeLook>()) {
        components["CFreeLook"] = serializeCFreeLook(entity->get<CFreeLook>());
    }

    // CTrait
    if (entity->has<CTrait>()) {
        components["CTrait"] = serializeCTrait(entity->get<CTrait>());
    }
    
    entityJson["components"] = components;
    return entityJson;
}

std::shared_ptr<Astral::Entity> SceneSerializer::deserializeEntity(
    const json& json,
    Astral::EntityManager& entityManager,
    IDMap& idMap) {
    
    std::string tag = json.value("tag", "entity");
    auto entity = entityManager.addEntity(tag);
    
    uint32_t oldId = json.value("id", 0u);
    uint32_t newId = entity->id();
    idMap[oldId] = newId;
    
    if (!json.contains("components")) {
        return entity;
    }
    
    const auto& components = json["components"];
    
    // CTransform
    if (components.contains("CTransform")) {
        entity->add<CTransform>(deserializeCTransform(components["CTransform"]));
    }
    
    // CShape
    if (components.contains("CShape")) {
        entity->add<CShape>(deserializeCShape(components["CShape"]));
    }
    
    // CBBox
    if (components.contains("CBBox")) {
        entity->add<CBBox>(deserializeCBBox(components["CBBox"]));
    }
    
    // CInput
    if (components.contains("CInput")) {
        entity->add<CInput>(deserializeCInput(components["CInput"]));
    }
    
    // CLifeSpan
    if (components.contains("CLifeSpan")) {
        entity->add<CLifeSpan>(deserializeCLifeSpan(components["CLifeSpan"]));
    }
    
    // CText
    if (components.contains("CText")) {
        entity->add<CText>(deserializeCText(components["CText"]));
    }
    
    // CSprite
    if (components.contains("CSprite")) {
        entity->add<CSprite>(deserializeCSprite(components["CSprite"]));
    }
    
    // CMesh
    if (components.contains("CMesh")) {
        entity->add<CMesh>(deserializeCMesh(components["CMesh"]));
    }
    
    // CCamera
    if (components.contains("CCamera")) {
        entity->add<CCamera>(deserializeCCamera(components["CCamera"]));
    }
    
    // CLight
    if (components.contains("CLight")) {
        entity->add<CLight>(deserializeCLight(components["CLight"]));
    }
    
    // CFreeLook
    if (components.contains("CFreeLook")) {
        entity->add<CFreeLook>(deserializeCFreeLook(components["CFreeLook"]));
    }

    // CTrait
    if (components.contains("CTrait")) {
        entity->add<CTrait>(deserializeCTrait(components["CTrait"]));
    }
    
    return entity;
}

void SceneSerializer::restoreHierarchy(const json& entitiesJson,
                                       Astral::EntityManager& entityManager,
                                       const IDMap& idMap) {
    // Tüm entity'leri al
    auto& allEntities = entityManager.getEntities();
    
    // ID'ye göre entity'leri hızlı erişim için map'e koy
    std::map<uint32_t, std::shared_ptr<Astral::Entity>> entityMap;
    for (auto& entity : allEntities) {
        entityMap[entity->id()] = entity;
    }
    
    // Her entity için parent-child ilişkilerini kur
    for (const auto& entityJson : entitiesJson) {
        uint32_t oldId = entityJson.value("id", 0u);
        
        // ID mapping'i kullanarak yeni ID'yi bul
        auto it = idMap.find(oldId);
        if (it == idMap.end()) continue;
        
        uint32_t newId = it->second;
        auto entityIt = entityMap.find(newId);
        if (entityIt == entityMap.end()) continue;
        
        auto entity = entityIt->second;
        
        if (!entity->has<CTransform>()) continue;
        
        auto& transform = entity->get<CTransform>();
        
        // Parent ID'sini restore et
        if (entityJson["components"].contains("CTransform")) {
            const auto& transformJson = entityJson["components"]["CTransform"];
            
            // Parent ID'si varsa
            if (transformJson.contains("parentId") && !transformJson["parentId"].is_null()) {
                uint32_t oldParentId = transformJson["parentId"].get<uint32_t>();
                auto parentIt = idMap.find(oldParentId);
                if (parentIt != idMap.end()) {
                    uint32_t newParentId = parentIt->second;
                    auto parentEntityIt = entityMap.find(newParentId);
                    if (parentEntityIt != entityMap.end()) {
                        transform.parent = parentEntityIt->second;
                    }
                }
            }
            
            // Children ID'lerini restore et
            if (transformJson.contains("childrenIds") && transformJson["childrenIds"].is_array()) {
                transform.children.clear();
                for (uint32_t oldChildId : transformJson["childrenIds"]) {
                    auto childIt = idMap.find(oldChildId);
                    if (childIt != idMap.end()) {
                        uint32_t newChildId = childIt->second;
                        auto childEntityIt = entityMap.find(newChildId);
                        if (childEntityIt != entityMap.end()) {
                            transform.children.push_back(childEntityIt->second);
                        }
                    }
                }
            }
        }
    }
}

bool SceneSerializer::serialize(const std::string& filepath,
                               Astral::EntityManager& entityManager) {
    try {
        json sceneJson;
        sceneJson["version"] = "1.0";
        sceneJson["metadata"] = json::object({
            {"timestamp", "2026-04-27"}
        });
        
        json entitiesJson = json::array();
        
        // Tüm entity'leri serialize et
        auto& entities = entityManager.getEntities();
        for (const auto& entity : entities) {
            if (entity->isActive()) {
                entitiesJson.push_back(serializeEntity(entity));
            }
        }
        
        sceneJson["entities"] = entitiesJson;
        
        // Dosyaya yaz
        std::ofstream file(filepath);
        if (!file.is_open()) {
            SDL_Log("SceneSerializer: Dosya açılamadı: %s", filepath.c_str());
            return false;
        }
        
        file << sceneJson.dump(2);
        file.close();
        
        SDL_Log("SceneSerializer: Sahne kaydedildi: %s", filepath.c_str());
        return true;
    }
    catch (const std::exception& e) {
        SDL_Log("SceneSerializer: Serialization hatası: %s", e.what());
        return false;
    }
}

bool SceneSerializer::deserialize(const std::string& filepath,
                                 Astral::EntityManager& entityManager) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            SDL_Log("SceneSerializer: Dosya açılamadı: %s", filepath.c_str());
            return false;
        }
        
        json sceneJson;
        file >> sceneJson;
        file.close();
        
        if (!sceneJson.contains("entities")) {
            SDL_Log("SceneSerializer: Geçersiz sahne dosyası: %s", filepath.c_str());
            return false;
        }
        
        IDMap idMap;
        const auto& entitiesJson = sceneJson["entities"];
        
        // Tüm entity'leri deserialize et
        for (const auto& entityJson : entitiesJson) {
            deserializeEntity(entityJson, entityManager, idMap);
        }
        
        // EntityManager'ı güncelle (pending entity'leri ekle)
        entityManager.update();
        
        // Hiyerarşiyi restore et
        restoreHierarchy(entitiesJson, entityManager, idMap);
        
        SDL_Log("SceneSerializer: Sahne yüklendi: %s", filepath.c_str());
        return true;
    }
    catch (const std::exception& e) {
        SDL_Log("SceneSerializer: Deserialization hatası: %s", e.what());
        return false;
    }
}

std::string SceneSerializer::createSnapshot(Astral::EntityManager& entityManager) {
    try {
        json sceneJson;
        sceneJson["version"] = "1.0";
        
        json entitiesJson = json::array();
        
        // Tüm entity'leri serialize et
        auto& entities = entityManager.getEntities();
        for (const auto& entity : entities) {
            if (entity->isActive()) {
                entitiesJson.push_back(serializeEntity(entity));
            }
        }
        
        sceneJson["entities"] = entitiesJson;
        
        return sceneJson.dump();
    }
    catch (const std::exception& e) {
        SDL_Log("SceneSerializer: Snapshot oluşturma hatası: %s", e.what());
        return "";
    }
}

bool SceneSerializer::restoreFromSnapshot(const std::string& jsonString,
                                         Astral::EntityManager& entityManager) {
    try {
        // Mevcut entity'leri temizle
        entityManager.clear();
        
        // Snapshot'tan parse et
        json sceneJson = json::parse(jsonString);
        
        if (!sceneJson.contains("entities")) {
            SDL_Log("SceneSerializer: Geçersiz snapshot");
            return false;
        }
        
        IDMap idMap;
        const auto& entitiesJson = sceneJson["entities"];
        
        // Tüm entity'leri deserialize et
        for (const auto& entityJson : entitiesJson) {
            deserializeEntity(entityJson, entityManager, idMap);
        }
        
        // EntityManager'ı güncelle
        entityManager.update();
        
        // Hiyerarşiyi restore et
        restoreHierarchy(entitiesJson, entityManager, idMap);
        
        SDL_Log("SceneSerializer: Snapshot restore edildi");
        return true;
    }
    catch (const std::exception& e) {
        SDL_Log("SceneSerializer: Snapshot restore hatası: %s", e.what());
        return false;
    }
}

bool SceneSerializer::serializeEntityToPrefab(const std::string& filepath, 
                                             const std::shared_ptr<Astral::Entity>& entity) {
    try {
        json entityJson = serializeEntity(entity);
        
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        
        file << entityJson.dump(2);
        file.close();
        
        SDL_Log("SceneSerializer: Prefab kaydedildi: %s", filepath.c_str());
        return true;
    }
    catch (const std::exception& e) {
        SDL_Log("SceneSerializer: Prefab serialization hatası: %s", e.what());
        return false;
    }
}

std::shared_ptr<Astral::Entity> SceneSerializer::loadPrefab(const std::string& filepath, 
                                                          Astral::EntityManager& entityManager) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) return nullptr;
        
        json entityJson;
        file >> entityJson;
        file.close();
        
        IDMap idMap;
        auto entity = deserializeEntity(entityJson, entityManager, idMap);
        
        // Tek bir entity olduğu için hiyerarşiyi kısıtlı olarak restore et
        // Eğer prefab içinde çocuk ID'leri varsa, onlar şu an sahnede yoksa bağlanamazlar.
        // Ama prefab sistemi genellikle hiyerarşiyi tek bir dosyada tutar (ileride genişletilebilir).
        
        entityManager.update();
        return entity;
    }
    catch (const std::exception& e) {
        SDL_Log("SceneSerializer: Prefab yükleme hatası: %s", e.what());
        return nullptr;
    }
}
