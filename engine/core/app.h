#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "core/entity_manager.h"
#include "core/system_manager.h"
#include "core/asset_manager.h"
#include "scene.h"
#include "renderer/graphics_device.h"
#include "renderer/renderer.h"
#include "systems/render_system.h"
#include "editor/editor_manager.h"
#include <memory>

class EditorManager;

// Uygulamanın ana iskeletini ve SDL yönetimini sağlayan sınıf.
// Oyunun yaşam döngüsü (başlatma, döngü, sonlandırma) burada kontrol edilir.
class App
{
public:
	App();
	~App();

	// Pencere oluşturma ve SDL sistemlerini ayağa kaldırma işlerini yapar.
	void init(const char* title, int width, int height);
	void run(); // Ana oyun döngüsünü başlatır.
	void shutdown(); // Kaynakları temizleyerek programı güvenli şekilde kapatır.
	void changeScene(std::unique_ptr<Scene> newScene);
	
	SDL_GPUDevice* getGPUDevice() const { return m_graphicsDevice ? m_graphicsDevice->getInternalDevice() : nullptr; }
	SDL_Window* getWindow() const { return m_window; }
	float getDeltaTime() const { return m_deltaTime; }
	float getFPS() const { return m_fps; }
	Astral::SystemManager& getSystemManager() { return m_systemManager; }
	Astral::AssetManager& getAssetManager() { return m_assetManager; }
	Astral::IRenderer* getRenderer() const { return m_renderer.get(); }

private:
	void activatePendingScene();
	void handleSceneStateTransition(SceneState nextState);
	void updateActiveScene(float deltaTime);
	void renderActiveScene();
	void registerEditorPanels();
	void configureInputCallbacks();
	Astral::CameraFrameData resolveActiveCamera() const;
	void initializeTraits();

	SDL_Window* m_window{ nullptr };
	std::unique_ptr<Astral::IGraphicsDevice> m_graphicsDevice{ nullptr };
	std::unique_ptr<Astral::IRenderer> m_renderer{ nullptr };
	
	bool			m_running{ false };
	std::unique_ptr<Scene> m_scene{ nullptr };
	std::unique_ptr<Scene> m_nextScene{ nullptr };
	Uint64 m_lastTime{ 0 }; // Delta time hesaplaması için önceki kare zamanı.
	float m_deltaTime{ 0.0f }; // Kareler arası geçen süre (donanımdan bağımsız hız için).
	float m_fps{ 0.0f }; // Saniye başına kare sayısı

	Astral::SystemManager m_systemManager;
	Astral::AssetManager m_assetManager;
	std::unique_ptr<EditorManager> m_editorManager;
	SceneState m_lastSceneState;
};
