#include "menu_scene.h"
#include "core/app.h"
#include "pong_game.h"
#include "ecs/components.h"
#include "systems/render_system.h"
#include "systems/text_system.h"

MenuScene::MenuScene(SDL_Renderer* renderer) : m_renderer(renderer)
{
}

MenuScene::~MenuScene()
{
	if (m_font)
	{
		TTF_CloseFont(m_font);
		m_font = nullptr;
	}
}

void MenuScene::init()
{
	m_font = TTF_OpenFont("assets/fonts/arial.ttf", 48);
	if (!m_font) SDL_Log("Failed to load font: %s", SDL_GetError());

	registerAction(SDLK_SPACE, "START");

	auto title = m_entityManager.addEntity("title");
	title->add<CTransform>(Vec2(150, 250), Vec2(0, 0));
	title->add<CText>("PRESS SPACE TO PLAY", m_font, SDL_Color{255, 255, 255, 255});

	m_entityManager.update();
}

void MenuScene::update(float deltaTime)
{
	m_entityManager.update();
}

void MenuScene::render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	TextSystem::update(m_entityManager, renderer);

	SDL_RenderPresent(renderer);
}

void MenuScene::sDoAction(const std::string& actionName, bool started)
{
	if (started && actionName == "START")
	{
		if (m_app)
		{
			m_app->changeScene(std::make_unique<PongGame>(m_renderer));
		}
	}
}
