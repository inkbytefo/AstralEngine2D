#pragma once
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "core/scene.h"

class MenuScene : public Scene
{
public:
	MenuScene(SDL_Renderer* renderer);
	~MenuScene();

	void init() override;
	void update(float deltaTime) override;
	void render(SDL_Renderer* renderer) override;
	bool isRunning() const override { return m_running; }

	void sDoAction(const std::string& actionName, bool started) override;

private:
	bool m_running{ true };
	TTF_Font* m_font{ nullptr };
	SDL_Renderer* m_renderer{ nullptr };
};
