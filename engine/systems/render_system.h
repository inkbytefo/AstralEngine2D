#pragma once
#include <SDL3/SDL.h>
#include "core/entity_manager.h"

// Çizim Sistemi: CShape ve CTransform bileşenlerini okuyup ekrana çizer.
class RenderSystem
{
public:
	static void update(EntityManager& entityManager, SDL_Renderer* renderer)
	{
		// Saydamlık (Alpha Blending) desteğini aç
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		for (auto& e : entityManager.getEntities())
		{
			if (e->has<CShape>() && e->has<CTransform>())
			{
				auto& transform = e->get<CTransform>();
				auto& shape = e->get<CShape>();

				SDL_FRect rect = { transform.pos.x, transform.pos.y, shape.width, shape.height };
				SDL_SetRenderDrawColor(renderer, shape.r, shape.g, shape.b, shape.a);
				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}
};