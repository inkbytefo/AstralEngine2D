#include "core/app.h"
#include <SDL3_ttf/SDL_ttf.h>


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

	m_lastTime = SDL_GetTicks();
	m_running = true;
	return true;
}

void App::run()
{
	// Oyunun sürekli çalışmasını sağlayan ana döngü.
	while (m_running && m_scene && m_scene->isRunning())
	{
		// Delta time hesaplayarak hareketin FPS'ten bağımsız, zamana bağlı olmasını sağlarız.
		Uint64 now = SDL_GetTicks();
		m_deltaTime = (now - m_lastTime) / 1000.0f;
		m_lastTime = now;

		m_scene->update(m_deltaTime); // Sahne güncellemesi (oyun mantığı)
		m_scene->render(m_renderer);   // Sahne çizimi (görselleştirme)
	}
}

void App::setScene(std::unique_ptr<Scene> newScene)
{
	m_scene = std::move(newScene);
	if (m_scene)
	{
		m_scene->init(); // Yeni sahne başlatılır.
	}
}


void App::shutdown()
{
	// reset the scene
	m_scene.reset();

	// Ayrılan bellekleri temizle ve SDL sistemlerini kapat.
	TTF_Quit();
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}