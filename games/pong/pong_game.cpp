#include "pong_game.h"
#include "ecs/components.h"
#include "systems/movement_system.h"
#include "systems/render_system.h"
#include "systems/lifespan_system.h"
#include "systems/text_system.h"
#include "core/sound_manager.h"
#include "core/level_loader.h"

PongGame::PongGame(SDL_Renderer* renderer) {
	// İstersen burada renderer ile Pong'a özel texture (görsel) yüklemeleri yapabilirsin
}

void PongGame::init()
{
	//add the font
	m_font = TTF_OpenFont("assets/fonts/arial.ttf", 48);
	if (!m_font) SDL_Log("Failed to load font: %s", SDL_GetError());

	// 1. Tuşları Aksiyonlara Bağla (Mapping)
	registerAction(SDLK_W,      "P1_UP");
	registerAction(SDLK_S,      "P1_DOWN");
	registerAction(SDLK_UP,     "P2_UP");
	registerAction(SDLK_DOWN,   "P2_DOWN");

	// Fabrikayı kullan!
	spawnScore("score_left",  Vec2(200, 50));
	spawnScore("score_right", Vec2(600, 50));
	
	// Veri odaklı (Data-Driven) tasarım: Seviyeyi JSON'dan yükle
	LevelLoader::loadLevel("assets/levels/pong_level.json", m_entityManager);

	SoundManager::getInstance().loadSound("hit", "assets/sounds/hit.wav");
	SoundManager::getInstance().loadSound("score", "assets/sounds/score.wav");

	m_entityManager.update(); // Eklenenleri dahil et
}

PongGame::~PongGame()
{
	if (m_font) 
	{
		TTF_CloseFont(m_font);
		m_font = nullptr;
	}
}



void PongGame::spawnScore(const std::string& tag, Vec2 pos) {
	auto score = m_entityManager.addEntity(tag);
	score->add<CTransform>(pos, Vec2(0, 0));
	score->add<CText>("0", m_font, SDL_Color{255, 255, 255, 255});
}

void PongGame::update(float deltaTime)
{
	m_entityManager.update();

	sUserInput();
	MovementSystem::update(m_entityManager, deltaTime);
	sPongCollision();
	
	for (auto& ball : m_entityManager.getEntities("ball"))
	{
		auto& bTrans = ball->get<CTransform>();
		auto& bShape = ball->get<CShape>();

		auto trail = m_entityManager.addEntity("trail");
		trail->add<CTransform>(bTrans.pos, Vec2(0, 0));
		trail->add<CShape>(bShape.width, bShape.height, bShape.r, bShape.g, bShape.b, 255);
		trail->add<CLifeSpan>(0.2f); // Yarım saniye ömrü var
	}
	LifespanSystem::update(m_entityManager, deltaTime);
}

void PongGame::render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	RenderSystem::update(m_entityManager, renderer);
	TextSystem::update(m_entityManager, renderer);

	SDL_RenderPresent(renderer);
}

void PongGame::sUserInput()
{
	// CInput bileşeni olan tüm varlıkları bul (Raketler)
	for (auto& e : m_entityManager.getEntities())
	{
		if (e->has<CInput>() && e->has<CTransform>())
		{
			auto& input = e->get<CInput>();
			auto& transform = e->get<CTransform>();

			transform.velocity.y = 0; // Her karede hızı sıfırla, tuşa basılıysa hız ver
			if (input.up)   transform.velocity.y -= 450; 
			if (input.down) transform.velocity.y += 450; 
		}
	}
}

void PongGame::sPongCollision()
{
	// Sadece 'ball' (top) etiketli nesneleri bul
	for (auto& ball : m_entityManager.getEntities("ball"))
	{
		auto& bTrans = ball->get<CTransform>();
		auto& bBox = ball->get<CBBox>();

		// Alt ve Üst Duvarlardan Sekme
		if (bTrans.pos.y <= 0 || bTrans.pos.y + bBox.height >= 600) {
			bTrans.velocity.y *= -1; // Yönü tersine çevir
		}

		// SKOR YÖNETİMİ: Top sağ veya sol ekrandan çıkarsa
		if (bTrans.pos.x < 0) {
			SoundManager::getInstance().playSound("score");
			m_scoreRight++; // Sağ taraf kazandı
			auto& scoreEntities = m_entityManager.getEntities("score_right");
			if (!scoreEntities.empty())
			{
				scoreEntities[0]->get<CText>().setText(std::to_string(m_scoreRight)); // Skoru güncelle
			}
			
			// Topu sıfırla
			bTrans.pos = Vec2(400, 300);
			bTrans.velocity = Vec2(250, 200);
		}
		else if (bTrans.pos.x > 800) {
			SoundManager::getInstance().playSound("score");
			m_scoreLeft++; // Sol taraf kazandı
			auto& scoreEntities = m_entityManager.getEntities("score_left");
			if (!scoreEntities.empty())
			{
				scoreEntities[0]->get<CText>().setText(std::to_string(m_scoreLeft)); // Skoru güncelle
			}
			
			// Topu sıfırla
			bTrans.pos = Vec2(400, 300);
			bTrans.velocity = Vec2(-250, 200); // Sol taraf kazandığı için topu ona doğru at
		}

		// RAKET ÇARPIŞMALARI (AABB Mantığı)
		for (auto& paddle : m_entityManager.getEntities())
		{
			// Raket olmayan nesneleri atla
			if (paddle->tag() != "paddle_left" && paddle->tag() != "paddle_right") continue;

			auto& pTrans = paddle->get<CTransform>();
			auto& pBox = paddle->get<CBBox>();

			// Kesişim (Collision) var mı?
			bool collision = 
				bTrans.pos.x < pTrans.pos.x + pBox.width &&
				bTrans.pos.x + bBox.width > pTrans.pos.x &&
				bTrans.pos.y < pTrans.pos.y + pBox.height &&
				bTrans.pos.y + bBox.height > pTrans.pos.y;

			if (collision) {
				SoundManager::getInstance().playSound("hit");
				bTrans.velocity.x *= -1.05f; // Yönü çevir ve her sekmede topu %5 hızlandır!
				
				// Topun raketin içine yapışmasını engellemek için küçük bir düzeltme:
				if (bTrans.velocity.x > 0)
					bTrans.pos.x = pTrans.pos.x + pBox.width;
				else
					bTrans.pos.x = pTrans.pos.x - bBox.width;
			}
		}
	}
}

void PongGame::sDoAction(const std::string& actionName, bool started)
{
	// Sol Raket Kontrolü
	auto leftPaddles = m_entityManager.getEntities("paddle_left");
	if (!leftPaddles.empty()) {
		auto& input = leftPaddles[0]->get<CInput>();
		if (actionName == "P1_UP")   input.up = started;
		if (actionName == "P1_DOWN") input.down = started;
	}

	// Sağ Raket Kontrolü
	auto rightPaddles = m_entityManager.getEntities("paddle_right");
	if (!rightPaddles.empty()) {
		auto& input = rightPaddles[0]->get<CInput>();
		if (actionName == "P2_UP")   input.up = started;
		if (actionName == "P2_DOWN") input.down = started;
	}
}