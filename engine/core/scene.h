#pragma once
#include "core/entity_manager.h"
#include <SDL3/SDL.h>
#include <map>
#include <string>

class App;

class Scene
{
public:
	virtual ~Scene() = default;

	virtual void init()						= 0; // Sahne başlatıldığında çağrılır (Oyuna özel kalacak)
	virtual void update(float deltaTime)	= 0; // Her kare güncellemesi (Oyuna özel kalacak)
	virtual bool isRunning() const			= 0; // Sahnenin çalışıp çalışmadığı

	virtual void render(SDL_GPURenderPass* renderPass) = 0;

	// Aksiyonu işle (Örn: "UP", true)
	virtual void sDoAction(const std::string& actionName, bool started) = 0;

	// Tuş -> Aksiyon eşleşmesini kaydet
	void registerAction(SDL_Keycode key, const std::string& actionName) {
		m_actionMap[key] = actionName;
	}

	const std::map<SDL_Keycode, std::string>& getActionMap() const {
		return m_actionMap;
	}

	EntityManager& getEntityManager() {
		return m_entityManager;
	}

	void setApp(App* app) { m_app = app; }

protected:
	// Artık her oyun ("PongGame", "MarioGame") kendi EntityManager'ını oluşturmak 
	// zorunda değil. "Scene" sınıfından miras (inherit) aldıklarında bu otomatik gelecek.
	EntityManager m_entityManager;
	std::map<SDL_Keycode, std::string> m_actionMap; // Tuş -> "UP", "DOWN" vb.
	App* m_app{ nullptr };
};