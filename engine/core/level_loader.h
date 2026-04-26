#pragma once

#include "core/entity_manager.h"
#include "ecs/components.h"
#include "core/json.hpp" // nlohmann/json
#include <fstream>
#include <SDL3/SDL.h>

class LevelLoader {
public:
    static bool loadLevel(const std::string& filePath, EntityManager& entityManager) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            SDL_Log("Level dosyasi bulunamadi: %s", filePath.c_str());
            return false;
        }

        nlohmann::json j;
        try {
            file >> j;
        } catch (const nlohmann::json::parse_error& e) {
            SDL_Log("JSON Parse Hatasi: %s", e.what());
            return false;
        }

        if (!j.contains("entities") || !j["entities"].is_array()) {
            SDL_Log("Gecersiz level formati: 'entities' array bulunamadi.");
            return false;
        }

        for (const auto& entityJson : j["entities"]) {
            std::string tag = entityJson.value("tag", "default");
            auto entity = entityManager.addEntity(tag);

            if (entityJson.contains("components")) {
                const auto& comps = entityJson["components"];

                if (comps.contains("CTransform")) {
                    auto& t = comps["CTransform"];
                    Vec2 pos(t["pos"][0].get<float>(), t["pos"][1].get<float>());
                    Vec2 vel(t["velocity"][0].get<float>(), t["velocity"][1].get<float>());
                    entity->add<CTransform>(pos, vel);
                }

                if (comps.contains("CShape")) {
                    auto& s = comps["CShape"];
                    entity->add<CShape>(
                        s["size"][0].get<float>(), 
                        s["size"][1].get<float>(), 
                        s["color"][0].get<uint8_t>(),
                        s["color"][1].get<uint8_t>(),
                        s["color"][2].get<uint8_t>(),
                        s["color"][3].get<uint8_t>()
                    );
                }

                if (comps.contains("CBBox")) {
                    auto& b = comps["CBBox"];
                    entity->add<CBBox>(b["size"][0].get<float>(), b["size"][1].get<float>());
                }

                if (comps.contains("CInput")) {
                    entity->add<CInput>();
                }
                
                // İleride CText veya CSprite gibi bileşenler buraya eklenebilir.
            }
        }

        SDL_Log("Level basariyla yuklendi: %s", filePath.c_str());
        return true;
    }
};
