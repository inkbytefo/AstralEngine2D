#pragma once
#include "SDL3/SDL.h"
#include "core/scene.h"

class PongGame : public Scene
{
public:
	// Constructor
	PongGame(SDL_Renderer* renderer);
	~PongGame();

	void init() override;
	void update(float deltaTime) override;
	void render(SDL_Renderer* renderer) override;
	bool isRunning() const override { return m_running; }

private:
	// Oyuna Özel Sistemler (Sadece Pong kurallarını içerir)
	void processEvents();
	void sUserInput();
	void sPongCollision();

	bool m_running{ true };
	int m_scoreLeft{ 0 };
	int m_scoreRight{ 0 };
	TTF_Font* m_font{ nullptr };
};