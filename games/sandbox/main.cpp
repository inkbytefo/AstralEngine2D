#include "core/app.h"
#include "sandbox_scene.h"

int main(int argc, char* argv[])
{
    App app;
    if (app.init("Astral Engine - Sandbox", 1280, 720)) {
        app.changeScene(std::make_unique<SandboxScene>());
        app.run();
    }
    app.shutdown();
    return 0;
}
