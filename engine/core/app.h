#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
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
	void changeScene(std::unique_ptr<Scene> newScene);
	SDL_GPUDevice* getGPUDevice() const { return m_gpuDevice; }
	SDL_Window* getWindow() const { return m_window; }
	float getDeltaTime() const { return m_deltaTime; }

private:
	SDL_Window* m_window{ nullptr };
	SDL_GPUDevice* m_gpuDevice{ nullptr };
	SDL_GPUTexture* m_depthTexture{ nullptr };
	bool			m_running{ false };
	std::unique_ptr<Scene> m_scene{ nullptr };
	std::unique_ptr<Scene> m_nextScene{ nullptr };
	Uint64 m_lastTime{ 0 }; // Delta time hesaplaması için önceki kare zamanı.
	float m_deltaTime{ 0.0f }; // Kareler arası geçen süre (donanımdan bağımsız hız için).

	void updateDepthTexture(int width, int height); // Depth buffer yönetimi
};