#include "core/app.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "core/asset_manager.h"
#include "core/sound_manager.h"
#include "systems/render_system.h"
#include "systems/transform_system.h"
#include "systems/physics_system.h"
#include "systems/camera_system.h"
#include "systems/input_system.h"
#include "systems/lifespan_system.h"
#include "systems/text_system.h"
#include "renderer/sdl3_graphics_device.h"
#include "renderer/sdl3_renderer.h"
#include "editor/editor_manager.h"

App::App() = default;
App::~App() = default;

bool App::init(const char* title, int width, int height)
{
	// SDL alt sistemlerini başlat.
	if (!SDL_Init(SDL_INIT_VIDEO)) return false;
	
	// SDL_ttf sistemini başlat.
	if (!TTF_Init()) return false;

	// İşletim sistemi seviyesinde bir pencere açar.
	m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
	if (!m_window) return false;

	// Grafik cihazını ve renderer'ı başlat
	m_graphicsDevice = std::make_unique<Astral::SDL3GraphicsDevice>();
	if (!m_graphicsDevice->init(m_window)) {
		return false;
	}

	m_renderer = std::make_unique<Astral::SDL3Renderer>(m_graphicsDevice.get());

	// Ses sistemini başlat
	if (!SoundManager::getInstance().init()) {
		SDL_Log("Ses sistemi baslatilamadi!");
		return false;
	}

	// AssetManager'ı başlat
	m_assetManager.setGPUDevice(m_graphicsDevice->getInternalDevice());

	m_lastTime = SDL_GetTicks();
	m_running = true;

	// --- Sistemleri Kaydet ---
	auto inputSys = std::make_unique<Astral::InputSystem>();
	inputSys->setApp(this);
	m_systemManager.addSystem(std::move(inputSys));

	m_systemManager.addSystem(std::make_unique<Astral::TransformSystem>());
	m_systemManager.addSystem(std::make_unique<Astral::PhysicsSystem>());
	m_systemManager.addSystem(std::make_unique<Astral::CameraSystem>());
	m_systemManager.addSystem(std::make_unique<Astral::LifespanSystem>());
	m_systemManager.addSystem(std::make_unique<Astral::TextSystem>());
	
	auto renderSys = std::make_unique<Astral::RenderSystem>();
	renderSys->setRenderer(m_renderer.get());
	renderSys->setAssetManager(&m_assetManager);
	renderSys->setWindow(m_window);
	m_systemManager.addSystem(std::move(renderSys));

	// EditorManager'ı başlat
	m_editorManager = std::make_unique<EditorManager>();
	SDL_GPUTextureFormat swapchainFormat = m_graphicsDevice->getSwapchainFormat(m_window);
	m_editorManager->init(m_window, m_graphicsDevice->getInternalDevice(), swapchainFormat);

	return true;
}

void App::run()
{
	while (m_running)
	{
		if (m_nextScene) {
			m_scene = std::move(m_nextScene);
			m_scene->setApp(this);
			m_scene->init();
			m_systemManager.init(m_scene->getEntityManager());

			if (auto* is = m_systemManager.getSystem<Astral::InputSystem>()) {
				is->setActionCallback([this](const std::string& name, bool started) {
					if (m_scene) m_scene->sDoAction(name, started);
				});
			}
		}

		if (!m_scene || !m_scene->isRunning()) {
			break;
		}

		Uint64 now = SDL_GetTicks();
		m_deltaTime = (now - m_lastTime) / 1000.0f;
		m_lastTime = now;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) m_running = false;
			m_systemManager.handleEvent(event);
			m_editorManager->processEvent(&event);
		}

		m_editorManager->newFrame();

		m_scene->update(m_deltaTime);
		
		SDL_FColor clearColor = { 0.1f, 0.15f, 0.2f, 1.0f };

		if (m_renderer->beginFrame()) {
			if (m_scene) {
				m_editorManager->drawEditor(m_scene->getEntityManager());
			}

			// SDL_GPU için PrepareDrawData mutlaka Render Pass dışında çağrılmalıdır.
			m_editorManager->prepare(m_renderer->getCurrentCommandBuffer());

			m_renderer->beginRenderPass(m_window, clearColor);
			if (m_scene) {
				m_systemManager.update(m_scene->getEntityManager(), m_deltaTime);
			}
			m_renderer->endRenderPass();

			// UI Pass: Derinlik tamponu kullanmadan (ImGui için)
			m_renderer->beginUIRenderPass(m_window);
			m_editorManager->render(m_renderer->getCurrentCommandBuffer(), m_renderer->getCurrentRenderPass());
			m_renderer->endRenderPass();
			m_renderer->endFrame();
		}
	}
}

void App::changeScene(std::unique_ptr<Scene> newScene)
{
	m_nextScene = std::move(newScene);
}

void App::shutdown()
{
	m_scene.reset();
	m_assetManager.cleanup();
	SoundManager::getInstance().cleanup();

	if (m_editorManager) {
		m_editorManager->shutdown();
		m_editorManager.reset();
	}

	TTF_Quit();
	m_renderer.reset();
	m_graphicsDevice.reset();
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}