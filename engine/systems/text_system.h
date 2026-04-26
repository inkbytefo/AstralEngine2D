#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "core/entity_manager.h"


/* Font System */
class TextSystem
{
public:
	static void update(EntityManager& entityManager, SDL_GPUCommandBuffer* commandBuffer)
	{
		// TODO: SDL_GPU ile metin çizimi yapılacak
		/*
		for (auto& e : entityManager.getEntities())
		...
		*/
	}
};