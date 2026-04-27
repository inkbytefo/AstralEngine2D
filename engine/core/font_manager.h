#pragma once
#include "asset_interfaces.h"
#include "gpu_resource.h"
#include <map>
#include <SDL3_ttf/SDL_ttf.h>

namespace Astral {

/**
 * @brief Concrete implementation of font management
 */
class FontManager : public IFontManager {
private:
    std::map<std::string, GpuFont> m_fonts;

public:
    FontManager() = default;

    bool loadFont(const std::string& name, const std::string& path, float size) override {
        TTF_Font* font = TTF_OpenFont(path.c_str(), static_cast<int>(size));
        if (!font) return false;

        m_fonts[name] = GpuFont(font, nullptr); // Font doesn't need device
        SDL_Log("FontManager: Font loaded: %s", name.c_str());
        return true;
    }

    TTF_Font* getFont(const std::string& name) const override {
        auto it = m_fonts.find(name);
        return (it != m_fonts.end()) ? it->second.get() : nullptr;
    }

    void cleanup() override {
        m_fonts.clear();
    }
};

} // namespace Astral