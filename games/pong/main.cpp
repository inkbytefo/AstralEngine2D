// SDLDers.cpp: Uygulamanın işletim sistemi tarafından tetiklenen ana giriş noktası.
//
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "common.h"
#include "core/app.h"
#include "pong_game.h"
#include "menu_scene.h"

using namespace std;

int main(int argc, char* argv[])
{
	// Uygulama sınıfını oluşturup yaşam döngüsünü başlatıyoruz.
	App app;

	if (!app.init("Astral Engine", 800, 600))
	{
		SDL_Log("Uygulama baslatilamadi: %s", SDL_GetError());
		return -1;
	}
	app.changeScene(make_unique<MenuScene>(app.getRenderer())); // Menu sahnesini başlat
	app.run();      // Ana döngü
	app.shutdown(); // Temizlik
	
	return 0;
}
