#pragma once
#include "core/entity_manager.h"
#include <SDL3/SDL.h>

class Scene
{
public:
	virtual ~Scene() = default;

	virtual void init()						= 0; // Sahne başlatıldığında çağrılır (Oyuna özel kalacak)
	virtual void update(float deltaTime)	= 0; // Her kare güncellemesi (Oyuna özel kalacak)
	virtual bool isRunning() const			= 0; // Sahnenin çalışıp çalışmadığı

	virtual void render(SDL_Renderer* renderer) = 0;

protected:
	// Artık her oyun ("PongGame", "MarioGame") kendi EntityManager'ını oluşturmak 
	// zorunda değil. "Scene" sınıfından miras (inherit) aldıklarında bu otomatik gelecek.
	EntityManager m_entityManager;
};