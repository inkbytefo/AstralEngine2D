#pragma once
#include "core/entity_manager.h"

// Hareket Sistemi: CTransform bileşeni olan tüm varlıkların pozisyonunu günceller.
class MovementSystem
{
public:
	static void update(EntityManager& entityManager, float deltaTime)
	{
		for (auto& e : entityManager.getEntities())
		{
			if (e->has<CTransform>())
			{
				e->get<CTransform>().pos += e->get<CTransform>().velocity * deltaTime;
			}
		}
	}
};