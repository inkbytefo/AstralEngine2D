#pragma once
#include "SDL3/SDL.h"
#include "core/scene.h"
#include <filesystem>

class PongGame : public Scene
{
public:
	PongGame(SDL_Renderer* renderer);

	void init() override;
	void update(float deltaTime) override;
	void render(SDL_Renderer* renderer) override;
	bool isRunning() const override { return m_running; }

	void sDoAction(const std::string& actionName, bool started) override;

private:
	// Oyuna Özel Sistemler (Sadece Pong kurallarını içerir)
	void sUserInput();
	void sPongCollision();

	bool m_running{ true };
	int m_scoreLeft{ 0 };
	int m_scoreRight{ 0 };
	std::filesystem::file_time_type m_lastLevelTime;
};