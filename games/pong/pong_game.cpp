#include "pong_game.h"
#include "ecs/components.h"
#include "systems/movement_system.h"
#include "systems/render_system.h"
#include "systems/lifespan_system.h"
#include "systems/text_system.h"

PongGame::PongGame(SDL_Renderer* renderer) {
	// İstersen burada renderer ile Pong'a özel texture (görsel) yüklemeleri yapabilirsin
}

void PongGame::init()
{
	//add the font
	m_font = TTF_OpenFont("assets/fonts/arial.ttf", 48);
	if (!m_font) SDL_Log("Failed to load font: %s", SDL_GetError());

	auto scoreLeft = m_entityManager.addEntity("score_left");
	scoreLeft->add<CTransform>(Vec2(200, 50), Vec2(0, 0));
	scoreLeft->add<CText>("0", m_font, SDL_Color{255,255,255,255});

	auto scoreRight = m_entityManager.addEntity("score_right");
	scoreRight->add<CTransform>(Vec2(600, 50), Vec2(0, 0));
	scoreRight->add<CText>("0", m_font, SDL_Color{ 255,255,255,255 });

	// Sol Raket (W/S tuşlarıyla kontrol edilecek)
	auto paddleLeft = m_entityManager.addEntity("paddle_left");
	paddleLeft->add<CTransform>(Vec2(50, 250), Vec2(0, 0));
	paddleLeft->add<CShape>(20, 100, 255, 255, 255, 255);
	paddleLeft->add<CBBox>(20, 100);
	paddleLeft->add<CInput>(); // Girdi alabileceğini belirtiyoruz

	// Sağ Raket (Yukarı/Aşağı ok tuşlarıyla kontrol edilecek)
	auto paddleRight = m_entityManager.addEntity("paddle_right");
	paddleRight->add<CTransform>(Vec2(730, 250), Vec2(0, 0));
	paddleRight->add<CShape>(20, 100, 255, 255, 255, 255);
	paddleRight->add<CBBox>(20, 100);
	paddleRight->add<CInput>();

	// Top (Sürekli hareket halinde)
	auto ball = m_entityManager.addEntity("ball");
	ball->add<CTransform>(Vec2(400, 300), Vec2(250, 200)); // x hızını 250, y hızını 200 yaptık
	ball->add<CShape>(15, 15, 255, 255, 0, 255); // Sarı renkli top
	ball->add<CBBox>(15, 15);

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

void PongGame::update(float deltaTime)
{
	processEvents();
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
			m_scoreRight++; // Sağ taraf kazandı
			auto& scoreEntities = m_entityManager.getEntities("score_right");
			if (!scoreEntities.empty())
			{
				scoreEntities[0]->get<CText>().setText(std::to_string(m_scoreRight)); // Skoru güncelle
			}
			bTrans.pos = Vec2(400, 300); // Merkeze koy
			bTrans.velocity = Vec2(250, 200); // Yeniden sağa at
		}
		else if (bTrans.pos.x > 800) {
			m_scoreLeft++; // Sol taraf kazandı
			auto& scoreEntities = m_entityManager.getEntities("score_left");
			if (!scoreEntities.empty())
			{
				scoreEntities[0]->get<CText>().setText(std::to_string(m_scoreLeft)); // Skoru güncelle
			}
			bTrans.pos = Vec2(400, 300);
			bTrans.velocity = Vec2(-250, 200); // Sola doğru at
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

void PongGame::processEvents()
{
	SDL_Event event{};
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_EVENT_QUIT) { m_running = false; }

		// Klavye olayları - Sadece CInput bileşenine veriyi yazar, hareketi sUserInput yapar.
		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
		{
			bool isDown = (event.type == SDL_EVENT_KEY_DOWN);
			
			// Güvenli varlık çekme
			auto leftPaddleList = m_entityManager.getEntities("paddle_left");
			auto rightPaddleList = m_entityManager.getEntities("paddle_right");

			if (!leftPaddleList.empty()) {
				auto& input = leftPaddleList[0]->get<CInput>();
				if (event.key.key == SDLK_W) input.up = isDown;
				if (event.key.key == SDLK_S) input.down = isDown;
			}

			if (!rightPaddleList.empty()) {
				auto& input = rightPaddleList[0]->get<CInput>();
				if (event.key.key == SDLK_UP) input.up = isDown;
				if (event.key.key == SDLK_DOWN) input.down = isDown;
			}
		}
	}
}