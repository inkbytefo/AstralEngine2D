#pragma once
#include "core/entity_manager.h"

// Ömür Sistemi: CLifespan'ı okur, zamanı düşürür, süresi biteni yok eder ve saydamlaştırır.
class LifespanSystem
{
public:
	static void update(EntityManager& entityManager, float deltaTime)
	{
		for (auto& e : entityManager.getEntities())
		{
			if (e->has<CLifeSpan>())
			{
				auto& lifespan = e->get<CLifeSpan>();
				lifespan.remaining -= deltaTime;

				// Ömrü bittiyse ölümü işaretle
				if (lifespan.remaining <= 0.0f)
				{
					e->destroy();
				}
				// Nesne ölmediyse ve şekli varsa, kalan ömrüne göre saydamlığını (alpha) azalt
				else if (e->has<CShape>())
				{
					float ratio = lifespan.remaining / lifespan.total;
					e->get<CShape>().a = static_cast<uint8_t>(255.0f * ratio);
				}
			}
		}
	}
};