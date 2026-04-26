#include "core/app.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "core/asset_manager.h"
#include "core/sound_manager.h"
#include "systems/render_system.h"



bool App::init(const char* title, int width, int height)
{
	// SDL alt sistemlerini başlat.
	if (!SDL_Init(SDL_INIT_VIDEO)) return false;
	
	// SDL_ttf sistemini başlat.
	if (!TTF_Init()) return false;

	// İşletim sistemi seviyesinde bir pencere açar.
	m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
	if (!m_window) return false;

	// SDL_GPU Cihazını oluştur (Vulkan ve SPIR-V'ye zorlayalım)
	m_gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
	if (!m_gpuDevice) {
		SDL_Log("SDL_GPU Cihazi olusturulamadi!");
		return false;
	}

	// Pencereyi GPU cihazına bağla
	if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice, m_window)) {
		SDL_Log("Pencere GPU cihazina baglanamadi!");
		return false;
	}

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
		
		// --- MODERN RENDER DÖNGÜSÜ (SDL_GPU) ---
		// 1. Komut Tamponu (Command Buffer) al
		SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
		if (!commandBuffer) continue;

		// 2. Swapchain Texture (Ekran yüzeyi) bekle ve al
		SDL_GPUTexture* swapchainTexture = nullptr;
		Uint32 w, h; // Texture boyutlarını alacağımız değişkenler
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, m_window, &swapchainTexture, &w, &h)) {
			SDL_SubmitGPUCommandBuffer(commandBuffer); // Hata durumunda boşa gönder
			continue;
		}

		if (swapchainTexture) {
			// 3. Render Pass Başlat (Ekranı temizleme işlemi burada yapılır)
			SDL_GPUColorTargetInfo colorTargetInfo = {};
			colorTargetInfo.texture = swapchainTexture;
			colorTargetInfo.clear_color = { 0.1f, 0.15f, 0.2f, 1.0f }; // Modern bir koyu mavi/gri
			colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
			colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

			SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);
			
			// SAHNEYİ RENDER PASS İLE ÇİZ!
			if (m_scene) {
				m_scene->render(renderPass);
				
				// RenderSystem ile 3D mesh'leri çiz
				Astral::RenderSystem::update(m_scene->getEntityManager(), commandBuffer, renderPass);
			}
			
			// 6. Render Pass Bitirme
			SDL_EndGPURenderPass(renderPass);
		}

		// 7. Komutları GPU'ya gönder (Swapchain texture alındığı için otomatik sunulur)
		SDL_SubmitGPUCommandBuffer(commandBuffer);
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
	Astral::AssetManager::getInstance().cleanup();

	// Ses sistemini temizle ve kapat
	SoundManager::getInstance().cleanup();

	// Ayrılan bellekleri temizle ve SDL sistemlerini kapat.
	TTF_Quit();
	SDL_ReleaseWindowFromGPUDevice(m_gpuDevice, m_window);
	SDL_DestroyGPUDevice(m_gpuDevice);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}