#pragma once
#include "core/entity_manager.h"
#include "core/json.hpp"
#include "core/asset_manager.h"
#include <fstream>
#include <iostream>

class LevelLoader {
public:
    static void loadLevel(EntityManager& entityManager, const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            SDL_Log("Level dosyasi bulunamadi: %s", filePath.c_str());
            return;
        }

        nlohmann::json j;
        try {
            file >> j;
        } catch (const nlohmann::json::parse_error& e) {
            SDL_Log("JSON Parse Hatasi: %s", e.what());
            return;
        }

        if (!j.contains("entities") || !j["entities"].is_array()) {
            SDL_Log("Gecersiz level formati: 'entities' array bulunamadi.");
            return;
        }

        for (auto& item : j["entities"]) {
            std::string tag = item.value("tag", "default");
            
            // Akıllı Güncelleme: Varlık varsa onu al, yoksa yeni oluştur!
            auto existingEntities = entityManager.getEntities(tag);
            std::shared_ptr<Entity> e;
            if (!existingEntities.empty()) {
                e = existingEntities[0]; // Var olanı güncelle
            } else {
                e = entityManager.addEntity(tag); // Yeni yarat
            }

            if (item.contains("components")) {
                auto& components = item["components"];

                if (components.contains("CTransform")) {
                    auto& c = components["CTransform"];
                    Vec2 pos(c["pos"][0].get<float>(), c["pos"][1].get<float>());
                    Vec2 vel(c["velocity"][0].get<float>(), c["velocity"][1].get<float>());
                    
                    if (e->has<CTransform>()) {
                        e->get<CTransform>().pos = pos;
                        e->get<CTransform>().velocity = vel;
                    } else {
                        e->add<CTransform>(pos, vel);
                    }
                }

                if (components.contains("CShape")) {
                    auto& c = components["CShape"];
                    if (e->has<CShape>()) {
                        auto& shape = e->get<CShape>();
                        shape.width = c["size"][0].get<float>(); 
                        shape.height = c["size"][1].get<float>();
                        shape.r = c["color"][0].get<uint8_t>(); 
                        shape.g = c["color"][1].get<uint8_t>(); 
                        shape.b = c["color"][2].get<uint8_t>(); 
                        shape.a = c["color"][3].get<uint8_t>();
                    } else {
                        e->add<CShape>(
                            c["size"][0].get<float>(), c["size"][1].get<float>(), 
                            c["color"][0].get<uint8_t>(), c["color"][1].get<uint8_t>(), 
                            c["color"][2].get<uint8_t>(), c["color"][3].get<uint8_t>()
                        );
                    }
                }

                if (components.contains("CBBox")) {
                    auto& c = components["CBBox"];
                    if (!e->has<CBBox>()) e->add<CBBox>(c["size"][0].get<float>(), c["size"][1].get<float>());
                }

                if (components.contains("CInput")) {
                    if (!e->has<CInput>()) e->add<CInput>();
                }

                // Yeni Eklenen CText Desteği!
                if (components.contains("CText")) {
                    auto& c = components["CText"];
                    std::string fontId = c["font_id"];
                    std::string text = c["text"];
                    SDL_Color color = { 
                        c["color"][0].get<uint8_t>(), 
                        c["color"][1].get<uint8_t>(), 
                        c["color"][2].get<uint8_t>(), 
                        c["color"][3].get<uint8_t>() 
                    };
                    
                    if (e->has<CText>()) {
                        auto& ctext = e->get<CText>();
                        ctext.font = AssetManager::getInstance().getFont(fontId);
                        ctext.color = color;
                        ctext.setText(text); // Metni de güncelleyelim!
                        ctext.needsUpdate = true; // Rengi/fontu değişirse texture yenilensin
                    } else {
                        e->add<CText>(text, AssetManager::getInstance().getFont(fontId), color);
                    }
                }
            }
        }
    }
};
