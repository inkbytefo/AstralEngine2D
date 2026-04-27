#include "core/app.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "imgui.h"
#include "core/asset_manager.h"
#include "core/sound_manager.h"
#include "core/error_handling.h"
#include "systems/render_system.h"
#include "systems/transform_system.h"
#include "systems/physics_system.h"
#include "systems/camera_system.h"
#include "systems/input_system.h"
#include "systems/lifespan_system.h"
#include "systems/text_system.h"
#include "systems/trait_system.h"
#include "core/scene_serializer.h"
#include "renderer/sdl3_graphics_device.h"
#include "renderer/sdl3_renderer.h"
#include "editor/editor_manager.h"
#include "editor/panels/scene_hierarchy_panel.h"
#include "editor/panels/properties_panel.h"
#include "editor/panels/viewport_panel.h"
#include "editor/panels/console_panel.h"
#include "editor/panels/content_browser_panel.h"

App::App() = default;
App::~App() = default;

void App::init(const char* title, int width, int height)
{
	try {
		// SDL alt sistemlerini başlat.
		SDL_ERROR_CHECK(SDL_Init(SDL_INIT_VIDEO));

		// SDL_ttf sistemini başlat.
		if (!TTF_Init()) {
			throw Astral::ErrorHandler::fromSDL("TTF_Init");
		}

		// İşletim sistemi seviyesinde bir pencere açar.
		m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
		if (!m_window) {
			throw Astral::ErrorHandler::fromSDL("SDL_CreateWindow");
		}

		// Grafik cihazını ve renderer'ı başlat
		m_graphicsDevice = std::make_unique<Astral::SDL3GraphicsDevice>();
		if (!m_graphicsDevice->init(m_window)) {
			ENGINE_ERROR_CTX(GPU, "Failed to initialize graphics device", "SDL3GraphicsDevice::init");
		}

		m_renderer = std::make_unique<Astral::SDL3Renderer>(m_graphicsDevice.get());

		// Ses sistemini başlat
		if (!SoundManager::getInstance().init()) {
			Astral::ErrorHandler::logWarning("Sound system initialization failed - continuing without audio");
		}

		// AssetManager'ı başlat
		m_assetManager.setGPUDevice(m_graphicsDevice->getInternalDevice());

		// EditorManager'ı başlat
		m_editorManager = std::make_unique<EditorManager>();
		m_editorManager->init(m_window, m_graphicsDevice->getInternalDevice(), m_graphicsDevice->getSwapchainFormat(m_window));
		m_lastSceneState = m_editorManager->getSceneState();

		// Sistemleri kaydet
		m_systemManager.addSystem(std::make_unique<Astral::InputSystem>());
		if (auto* is = m_systemManager.getSystem<Astral::InputSystem>()) {
			is->setApp(this);
		}
		
		m_systemManager.addSystem(std::make_unique<Astral::TransformSystem>());
		m_systemManager.addSystem(std::make_unique<Astral::PhysicsSystem>());
		m_systemManager.addSystem(std::make_unique<Astral::CameraSystem>());
		m_systemManager.addSystem(std::make_unique<Astral::LifespanSystem>());
		m_systemManager.addSystem(std::make_unique<Astral::TraitSystem>());

		auto renderSystem = std::make_unique<Astral::RenderSystem>();
		renderSystem->setAssetManager(&m_assetManager);
		renderSystem->setWindow(m_window);
		renderSystem->setRenderer(m_renderer.get());
		m_systemManager.addSystem(std::move(renderSystem));

		m_lastTime = SDL_GetTicks();
		m_running = true;

		Astral::ErrorHandler::logInfo("Application initialized successfully");
	}
	catch (const Astral::EngineError& e) {
		Astral::ErrorHandler::logError(e);
		shutdown(); // Cleanup on error
		throw; // Re-throw to caller
	}
}

void App::run()
{
	try {
		while (m_running)
		{
		activatePendingScene();

		if (!m_scene || !m_scene->isRunning()) {
			break;
		}

		Uint64 now = SDL_GetTicks();
		m_deltaTime = (now - m_lastTime) / 1000.0f;
		m_lastTime = now;
		
		// FPS hesapla
		if (m_deltaTime > 0.0f)
		{
			m_fps = 1.0f / m_deltaTime;
		}

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) m_running = false;
			m_systemManager.handleEvent(event);
			m_editorManager->processEvent(&event);
		}

		m_editorManager->newFrame();
		handleSceneStateTransition(m_editorManager->getSceneState());
		updateActiveScene(m_deltaTime);
		renderActiveScene();
		}
	}
	catch (const Astral::EngineError& e) {
		Astral::ErrorHandler::logError(e);
		if (Astral::ErrorHandler::canRecover(e)) {
			Astral::ErrorHandler::attemptRecovery(e);
		} else {
			Astral::ErrorHandler::logError(Astral::EngineError(Astral::EngineError::Category::Configuration,
				"Fatal error occurred, shutting down application"));
			m_running = false;
		}
	}
	catch (const std::exception& e) {
		Astral::ErrorHandler::logError(Astral::EngineError(Astral::EngineError::Category::Configuration,
			std::string("Unexpected error: ") + e.what()));
		m_running = false;
	}
	catch (...) {
		Astral::ErrorHandler::logError(Astral::EngineError(Astral::EngineError::Category::Configuration,
			"Unknown error occurred"));
		m_running = false;
	}
}

void App::changeScene(std::unique_ptr<Scene> newScene)
{
	m_nextScene = std::move(newScene);
}

void App::activatePendingScene()
{
	if (!m_nextScene) {
		return;
	}

	m_scene = std::move(m_nextScene);
	m_scene->setApp(this);
	m_scene->init();
	m_systemManager.init(m_scene->getEntityManager());
	registerEditorPanels();
	configureInputCallbacks();
	m_lastSceneState = m_editorManager->getSceneState();
}

void App::handleSceneStateTransition(SceneState nextState)
{
	if (!m_scene || nextState == m_lastSceneState) {
		return;
	}

	if (nextState == SceneState::Play && m_lastSceneState == SceneState::Edit) {
		m_editorManager->clearSelection();
		m_editorManager->setSceneSnapshot(SceneSerializer::createSnapshot(m_scene->getEntityManager()));
		initializeTraits();
	}
	else if (nextState == SceneState::Edit && m_lastSceneState != SceneState::Edit) {
		m_editorManager->clearSelection();
		const std::string& snapshot = m_editorManager->getSceneSnapshot();
		if (!snapshot.empty()) {
			SceneSerializer::restoreFromSnapshot(snapshot, m_scene->getEntityManager());
		}
	}

	m_lastSceneState = nextState;
}

void App::updateActiveScene(float deltaTime)
{
	if (!m_scene) {
		return;
	}

	if (m_editorManager->getSceneState() == SceneState::Play) {
		m_scene->update(deltaTime);
	}
}

void App::renderActiveScene()
{
	if (!m_scene || !m_renderer->beginFrame()) {
		return;
	}

	SDL_FColor clearColor = { 0.1f, 0.15f, 0.2f, 1.0f };
	SceneState sceneState = m_editorManager->getSceneState();

	m_renderer->beginScenePass(clearColor);
	if (auto* renderSys = m_systemManager.getSystem<Astral::RenderSystem>()) {
		renderSys->setCameraData(resolveActiveCamera());
	}

	if (sceneState == SceneState::Play) {
		m_systemManager.update(m_scene->getEntityManager(), m_deltaTime);
	}
	else {
		if (auto* transformSys = m_systemManager.getSystem<Astral::TransformSystem>()) {
			transformSys->update(m_scene->getEntityManager(), m_deltaTime);
		}
		if (auto* renderSys = m_systemManager.getSystem<Astral::RenderSystem>()) {
			renderSys->update(m_scene->getEntityManager(), m_deltaTime);
		}
	}
	
	m_renderer->endScenePass();
	m_editorManager->drawEditor(m_scene->getEntityManager(), m_renderer->getSceneTexture(), m_renderer.get());

	int drawCalls = 0;
	if (auto* renderSys = m_systemManager.getSystem<Astral::RenderSystem>()) {
		drawCalls = renderSys->getDrawCallCount();
	}
	m_editorManager->setDebugInfo(m_fps, drawCalls);
	m_editorManager->prepare(m_renderer->getCurrentCommandBuffer());

	m_renderer->beginUIPass(m_window);
	m_editorManager->render(m_renderer->getCurrentCommandBuffer(), m_renderer->getCurrentRenderPass());
	m_renderer->endUIPass();
	m_renderer->endFrame();

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void App::registerEditorPanels()
{
	m_editorManager->registerPanel(std::make_unique<SceneHierarchyPanel>(&m_scene->getEntityManager()));
	m_editorManager->registerPanel(std::make_unique<PropertiesPanel>());

	auto viewportPanel = std::make_unique<ViewportPanel>(m_renderer.get(), &m_scene->getEntityManager());
	viewportPanel->setEditorManager(m_editorManager.get());
	m_editorManager->registerPanel(std::move(viewportPanel));

	m_editorManager->registerPanel(std::make_unique<ConsolePanel>());
	m_editorManager->registerPanel(std::make_unique<ContentBrowserPanel>());
}

void App::configureInputCallbacks()
{
	if (auto* is = m_systemManager.getSystem<Astral::InputSystem>()) {
		is->setActionCallback([this](const std::string& name, bool started) {
			if (m_scene) {
				m_scene->sDoAction(name, started);
			}
		});
	}
}

Astral::CameraFrameData App::resolveActiveCamera() const
{
	Astral::CameraFrameData cameraData;

	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 pos;
	if (!m_editorManager->getEditorCameraMatrices(view, proj, pos)) {
		return cameraData;
	}

	cameraData.view = view;
	cameraData.projection = proj;
	cameraData.position = pos;
	cameraData.isValid = true;
	return cameraData;
}

void App::initializeTraits()
{
	for (auto& e : m_scene->getEntityManager().getEntities()) {
		if (!e->has<CTrait>()) {
			continue;
		}

		for (auto& trait : e->get<CTrait>().traits) {
			trait->onInit(*e);
		}
	}
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
