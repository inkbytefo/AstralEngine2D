#include "core/app.h"


bool App::init(const char* title, int width, int height)
{
	// SDL alt sistemlerini başlat.
	if (!SDL_Init(SDL_INIT_VIDEO)) return false;
	
	// İşletim sistemi seviyesinde bir pencere açar.
	m_window = SDL_CreateWindow(title, width, height, 0);
	if (!m_window) return false;

	// Çizim işlemlerini yapacak olan donanım hızlandırmalı renderer'ı oluşturur.
	m_renderer = SDL_CreateRenderer(m_window, nullptr);
	if (!m_renderer) return false;

	m_lastTime = SDL_GetTicks();
	m_running = true;

	// left paddle
	auto paddleLeft			= m_entityManager.addEntity("paddle_left");
	paddleLeft->transform	= CTransform({ 50.0f, 300.0f }, { 0.0f, 0.0f });
	paddleLeft->shape		= CShape(20.0f, 100.0f, 0, 0, 255, 255);


	// right paddle
	auto paddleRight = m_entityManager.addEntity("paddle_right");
	paddleRight->transform = CTransform({ 750.0f, 300.0f }, { 0.0f, 0.0f });
	paddleRight->shape = CShape(20.0f, 100.0f, 255, 0, 0, 255);

// ball
auto ball = m_entityManager.addEntity("ball");
ball->transform = CTransform({ 400.0f, 300.0f }, { 150.0f, 150.0f });
ball->shape = CShape(20.0f, 20.0f, 255, 255, 255, 255);



// Yeni eklenen varlığın hemen ilk karede hazır olması için güncelleme yapılır.
m_entityManager.update();

return true;
}

void App::run()
{
	// Oyunun sürekli çalışmasını sağlayan ana döngü.
	while (m_running)
	{
		// Delta time hesaplayarak hareketin FPS'ten bağımsız, zamana bağlı olmasını sağlarız.
		Uint64 now = SDL_GetTicks();
		m_deltaTime = (now - m_lastTime) / 1000.0f;
		m_lastTime = now;

		processEvents();
		update();
		render();
	}
}

void App::processEvents()
{
	SDL_Event event{ 0 };
	// İşletim sisteminden gelen tüm olayları (kapatma, tuş basımı vb.) kuyruktan çeker.
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			m_running = false;
			break;
		default:
			break;
		}
	}

}

void App::update()
{
	// Varlık sisteminin bekleyen işlemlerini halleder.
	m_entityManager.update();


	// Klavye durumuna göre oyuncu hareketini belirler.
	const bool* keys = SDL_GetKeyboardState(nullptr);

	// left paddle W/S
	for (auto& e : m_entityManager.getEntities("paddle_left"))
	{
		e->transform.velocity = { 0.0f, 0.0f };
		if (keys[SDL_SCANCODE_W]) e->transform.velocity.y = -200.0f;
		if (keys[SDL_SCANCODE_S]) e->transform.velocity.y = 200.0f;
		// Konumu hız ve geçen süre (delta time) ile güncelleyerek akıcı hareket sağlarız.
		e->transform.pos += e->transform.velocity * m_deltaTime;

		if (e->transform.pos.y < 0.0f)
			e->transform.pos.y = 0.0f;
		if (e->transform.pos.y > 600.0f - e->shape.height)
			e->transform.pos.y = 600.0f - e->shape.height;

	}


	// right paddle Up/Down
	for (auto& e : m_entityManager.getEntities("paddle_right"))
	{
		e->transform.velocity = { 0.0f, 0.0f };
		if (keys[SDL_SCANCODE_UP]) e->transform.velocity.y = -200.0f;
		if (keys[SDL_SCANCODE_DOWN]) e->transform.velocity.y = 200.0f;
		// Konumu hız ve geçen süre (delta time) ile güncelleyerek akıcı hareket sağlarız.
		e->transform.pos += e->transform.velocity * m_deltaTime;

		if (e->transform.pos.y < 0.0f)
			e->transform.pos.y = 0.0f;
		if (e->transform.pos.y > 600.0f - e->shape.height)
			e->transform.pos.y = 600.0f - e->shape.height;
	}

	// ball
	for (auto& e : m_entityManager.getEntities("ball"))
	{
		e->transform.pos += e->transform.velocity * m_deltaTime;
		if (e->transform.pos.y < 0.0f || e->transform.pos.y > 600.0f - e->shape.height)
			e->transform.velocity.y = -e->transform.velocity.y;
		if (e->transform.pos.x < 0.0f || e->transform.pos.x > 800.0f - e->shape.width)
			e->transform.velocity.x = -e->transform.velocity.x;
	}

	// top - paddle çarpışması
	for (auto& ball : m_entityManager.getEntities("ball"))
	{
		for (auto& paddle : m_entityManager.getEntities("paddle_left"))
		{
			if (checkCollision(ball->transform, ball->shape,
				paddle->transform, paddle->shape))
			{
				ball->transform.velocity.x = std::abs(ball->transform.velocity.x);
			}
		}

		for (auto& paddle : m_entityManager.getEntities("paddle_right"))
		{
			if (checkCollision(ball->transform, ball->shape,
				paddle->transform, paddle->shape))
			{
				ball->transform.velocity.x = -std::abs(ball->transform.velocity.x);
			}
		}
	}

}

void App::render()
{
	// Arka planı beyaz ile temizleyerek yeni kareye hazır hale getiririz.
	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	SDL_RenderClear(m_renderer);


	// Tüm varlıkları teker teker dolaşarak ekrandaki yerlerine çizer.
	for (auto& e : m_entityManager.getEntities())
	{
		SDL_FRect rect = { e->transform.pos.x, e->transform.pos.y, e->shape.width, e->shape.height };
		SDL_SetRenderDrawColor(m_renderer, e->shape.r, e->shape.g, e->shape.b, e->shape.a);
		SDL_RenderFillRect(m_renderer, &rect);
	}


	// Çizimleri ekrana yansıtarak kullanıcının görmesini sağlarız (Double buffering).
	SDL_RenderPresent(m_renderer);
}

void App::shutdown()
{
	// Ayrılan bellekleri temizle ve SDL sistemlerini kapat.
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}