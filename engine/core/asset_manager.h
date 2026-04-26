#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
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

    bool loadFont(const std::string& name, const std::string& path, int size) {
        TTF_Font* font = TTF_OpenFont(path.c_str(), size);
        if (!font) {
            SDL_Log("Font yuklenemedi: %s - Hata: %s", path.c_str(), SDL_GetError());
            return false;
        }
        m_fonts[name] = font;
        return true;
    }

    TTF_Font* getFont(const std::string& name) {
        if (m_fonts.find(name) == m_fonts.end()) return nullptr;
        return m_fonts[name];
    }

    void cleanup() {
        for (auto& [name, tex] : m_textures) {
            SDL_DestroyTexture(tex);
        }
        m_textures.clear();

        for (auto& [name, font] : m_fonts) {
            TTF_CloseFont(font);
        }
        m_fonts.clear();
    }

private:
    AssetManager() = default;
    ~AssetManager() = default;
    std::map<std::string, SDL_Texture*> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;
};