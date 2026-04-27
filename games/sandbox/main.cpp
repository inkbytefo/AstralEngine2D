#include "core/app.h"
#include "sandbox_scene.h"
#include "core/error_handling.h"

int main(int argc, char* argv[])
{
    try {
        App app;
        app.init("Astral Engine - Sandbox", 1280, 720);
        app.changeScene(std::make_unique<SandboxScene>());
        app.run();
        app.shutdown();
        return 0;
    }
    catch (const Astral::EngineError& e) {
        Astral::ErrorHandler::logError(e);
        return 1; // Error exit code
    }
    catch (const std::exception& e) {
        Astral::ErrorHandler::logError(Astral::EngineError(
            Astral::EngineError::Category::Configuration,
            std::string("Unexpected error: ") + e.what()));
        return 1;
    }
    catch (...) {
        Astral::ErrorHandler::logError(Astral::EngineError(
            Astral::EngineError::Category::Configuration,
            "Unknown error occurred"));
        return 1;
    }
}
