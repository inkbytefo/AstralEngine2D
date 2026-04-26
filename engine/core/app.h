#pragma once
#include <SDL3/SDL.h>
#include "math/vec2.h"
#include "core/entity_manager.h"
#include "scene.h"

// Uygulamanın ana iskeletini ve SDL yönetimini sağlayan sınıf.
// Oyunun yaşam döngüsü (başlatma, döngü, sonlandırma) burada kontrol edilir.
class App
{
public:
	// Pencere oluşturma ve SDL sistemlerini ayağa kaldırma işlerini yapar.
	bool init(const char* title, int width, int height);
	void run(); // Ana oyun döngüsünü başlatır.
	void shutdown(); // Kaynakları temizleyerek programı güvenli şekilde kapatır.
	void setScene(std::unique_ptr<Scene> newScene);
	SDL_Renderer* getRenderer() const { return m_renderer; }

private:
	SDL_Window* m_window{ nullptr };
	SDL_Renderer* m_renderer{ nullptr };
	bool			m_running{ false };
	std::unique_ptr<Scene> m_scene{ nullptr };
	Uint64 m_lastTime{ 0 }; // Delta time hesaplaması için önceki kare zamanı.
	float m_deltaTime{ 0.0f }; // Kareler arası geçen süre (donanımdan bağımsız hız için).
};