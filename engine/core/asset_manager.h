#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <map>
#include <string>

class AssetManager {
public:
    static AssetManager& getInstance() {
        static AssetManager instance;
        return instance;
    }

    bool loadTexture(SDL_Renderer* renderer, const std::string& name, const std::string& path) {
        SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
        if (!tex) {
            SDL_Log("Texture yuklenemedi: %s - Hata: %s", path.c_str(), SDL_GetError());
            return false;
        }
        m_textures[name] = tex;
        return true;
    }

    SDL_Texture* getTexture(const std::string& name) {
        if (m_textures.find(name) == m_textures.end()) return nullptr;
        return m_textures[name];
    }

    void cleanup() {
        for (auto& [name, tex] : m_textures) {
            SDL_DestroyTexture(tex);
        }
        m_textures.clear();
    }

private:
    AssetManager() = default;
    std::map<std::string, SDL_Texture*> m_textures;
};