#pragma once
#include <SDL3/SDL.h>
#include "core/entity_manager.h"

// Çizim Sistemi: CShape, CSprite ve CTransform bileşenlerini okuyup ekrana çizer.
class RenderSystem
{
public:
    static void update(EntityManager& entityManager, SDL_Renderer* renderer)
    {
        // Saydamlık (Alpha Blending) desteğini aç
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        for (auto& e : entityManager.getEntities())
        {
            if (!e->has<CTransform>()) continue;
            auto& transform = e->get<CTransform>();

            // 1. Önce Sprite Çizimi (Eğer varsa)
            if (e->has<CSprite>())
            {
                auto& sprite = e->get<CSprite>();
                
                // Hedef dikdörtgen (Ekranda nereye çizilecek?)
                SDL_FRect dstRect = { transform.pos.x, transform.pos.y, sprite.srcRect.w, sprite.srcRect.h };

                // Eğer CShape varsa boyutu oradan al (Scale/Boyutlandırma desteği)
                if (e->has<CShape>())
                {
                    dstRect.w = e->get<CShape>().width;
                    dstRect.h = e->get<CShape>().height;
                }

                // SDL3'ün yeni render fonksiyonu
                // nullptr: Merkezden döndür (center), SDL_FLIP_NONE: Aynalama yapma
                SDL_RenderTextureRotated(renderer, sprite.texture, &sprite.srcRect, &dstRect, 
                                        (double)sprite.angle, nullptr, SDL_FLIP_NONE);
            }
            // 2. Eğer Sprite yoksa ama Shape varsa, Şekil Çizimi
            else if (e->has<CShape>())
            {
                auto& shape = e->get<CShape>();
                SDL_FRect rect = { transform.pos.x, transform.pos.y, shape.width, shape.height };
                SDL_SetRenderDrawColor(renderer, shape.r, shape.g, shape.b, shape.a);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
};