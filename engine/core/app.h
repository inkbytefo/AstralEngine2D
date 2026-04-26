#pragma once
#include <SDL3/SDL.h>
#include "math/vec2.h"
#include "core/entity_manager.h"


// Uygulamanın ana iskeletini ve SDL yönetimini sağlayan sınıf.
// Oyunun yaşam döngüsü (başlatma, döngü, sonlandırma) burada kontrol edilir.
class App
{
public:
	// Pencere oluşturma ve SDL sistemlerini ayağa kaldırma işlerini yapar.
	bool init(const char* title, int width, int height);
	// Ana oyun döngüsünü başlatır.
	void run();
	// Kaynakları temizleyerek programı güvenli şekilde kapatır.
	void shutdown();

private:
	// Kullanıcı girdilerini (klavye, mouse, kapatma) yakalar.
	void processEvents();
	// Fizik hesaplamaları ve oyun mantığının güncellendiği aşama.
	void update();
	// Ekrana çizim işlemlerinin yapıldığı aşama.
	void render();

	SDL_Window* m_window{ nullptr };
	SDL_Renderer* m_renderer{ nullptr };
	bool			m_running{ false };

	// Oyundaki tüm varlıkların yöneticisi.
	EntityManager m_entityManager;

	Uint64 m_lastTime{ 0 }; // Delta time hesaplaması için önceki kare zamanı.
	float m_deltaTime{ 0.0f }; // Kareler arası geçen süre (donanımdan bağımsız hız için).
};