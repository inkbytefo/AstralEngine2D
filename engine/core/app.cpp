#include "core/app.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "core/asset_manager.h"
#include "core/sound_manager.h"



bool App::init(const char* title, int width, int height)
{
	// SDL alt sistemlerini başlat.
	if (!SDL_Init(SDL_INIT_VIDEO)) return false;
	
	// SDL_ttf sistemini başlat.
	if (!TTF_Init()) return false;

	// İşletim sistemi seviyesinde bir pencere açar.
	m_window = SDL_CreateWindow(title, width, height, 0);
	if (!m_window) return false;

	// Çizim işlemlerini yapacak olan donanım hızlandırmalı renderer'ı oluşturur.
	m_renderer = SDL_CreateRenderer(m_window, nullptr);
	if (!m_renderer) return false;

	// Ses sistemini başlat
	if (!SoundManager::getInstance().init()) {
		SDL_Log("Ses sistemi baslatilamadi!");
		return false;
	}

	m_lastTime = SDL_GetTicks();
	m_running = true;
	return true;
}

void App::run()
{
	// Oyunun sürekli çalışmasını sağlayan ana döngü.
	while (m_running)
	{
		// --- SAHNE DEĞİŞİM KONTROLÜ (Karenin başında güvenli değişim) ---
		if (m_nextScene) {
			m_scene = std::move(m_nextScene);
			m_scene->setApp(this);
			m_scene->init();
		}

		if (!m_scene || !m_scene->isRunning()) {
			break; // Sahne bitti veya yok, çık
		}

		// Delta time hesaplayarak hareketin FPS'ten bağımsız, zamana bağlı olmasını sağlarız.
		Uint64 now = SDL_GetTicks();
		m_deltaTime = (now - m_lastTime) / 1000.0f;
		m_lastTime = now;

		// --- GİRDİ YÖNETİMİ (Input Handling) ---
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) m_running = false;

			if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
				bool started = (event.type == SDL_EVENT_KEY_DOWN);
				SDL_Keycode key = event.key.key;

				// Sahnenin Action Map'inde bu tuş var mı?
				auto& actionMap = m_scene->getActionMap();
				if (actionMap.find(key) != actionMap.end()) {
					// Varsa aksiyonu sahneye gönder
					m_scene->sDoAction(actionMap.at(key), started);
				}
			}
		}

		m_scene->update(m_deltaTime); // Sahne güncellemesi (oyun mantığı)
		m_scene->render(m_renderer);   // Sahne çizimi (görselleştirme)
	}
}

void App::changeScene(std::unique_ptr<Scene> newScene)
{
	m_nextScene = std::move(newScene);
}


void App::shutdown()
{
	// reset the scene
	m_scene.reset();

	// Önce texture'ları sil (AssetManager)
	AssetManager::getInstance().cleanup();

	// Ses sistemini temizle ve kapat
	SoundManager::getInstance().cleanup();

	// Ayrılan bellekleri temizle ve SDL sistemlerini kapat.
	TTF_Quit();
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}