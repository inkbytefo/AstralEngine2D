#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "core/entity_manager.h"


/* Font System */
class TextSystem
{
public:
	static void update(EntityManager& entityManager, SDL_Renderer* renderer)
	{
		for (auto& e : entityManager.getEntities())
		{
			if (e->has<CText>() && e->has<CTransform>())
			{
				auto& cText = e->get<CText>();
				auto& cTrans = e->get<CTransform>();

				if (!cText.font || cText.text.empty()) continue;

				// Eğer metin değiştiyse veya henüz texture oluşturulmadıysa yeni texture oluştur
				if (cText.needsUpdate || !cText.texture)
				{
					if (cText.texture) SDL_DestroyTexture(cText.texture);
					
					SDL_Surface* surface = TTF_RenderText_Solid(cText.font, cText.text.c_str(), cText.text.length(), cText.color);
					if (surface)
					{
						cText.texture = SDL_CreateTextureFromSurface(renderer, surface);
						cText.width = (float)surface->w;
						cText.height = (float)surface->h;
						SDL_DestroySurface(surface);
					}
					cText.needsUpdate = false;
				}

				// Texture varsa ekrana çiz
				if (cText.texture)
				{
					SDL_FRect dstRect = { cTrans.pos.x, cTrans.pos.y, cText.width, cText.height };
					SDL_RenderTexture(renderer, cText.texture, nullptr, &dstRect);
				}
			}
		}
	}
};