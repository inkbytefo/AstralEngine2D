#include "error_handling.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <sstream>

namespace Astral {

void ErrorHandler::logError(const EngineError& error) {
    std::stringstream ss;
    ss << "[ERROR] ";

    switch (error.category) {
        case EngineError::Category::SDL: ss << "[SDL] "; break;
        case EngineError::Category::GPU: ss << "[GPU] "; break;
        case EngineError::Category::Asset: ss << "[ASSET] "; break;
        case EngineError::Category::ECS: ss << "[ECS] "; break;
        case EngineError::Category::Rendering: ss << "[RENDER] "; break;
        case EngineError::Category::FileSystem: ss << "[FILE] "; break;
        case EngineError::Category::Memory: ss << "[MEMORY] "; break;
        case EngineError::Category::Configuration: ss << "[CONFIG] "; break;
    }

    ss << error.what();
    if (!error.context.empty()) {
        ss << " (Context: " << error.context << ")";
    }

    std::cerr << ss.str() << std::endl;
    SDL_Log("%s", ss.str().c_str());
}

void ErrorHandler::logWarning(std::string_view message) {
    std::stringstream ss;
    ss << "[WARNING] " << message;
    std::cout << ss.str() << std::endl;
    SDL_Log("%s", ss.str().c_str());
}

void ErrorHandler::logInfo(std::string_view message) {
    std::stringstream ss;
    ss << "[INFO] " << message;
    std::cout << ss.str() << std::endl;
    SDL_Log("%s", ss.str().c_str());
}

EngineError ErrorHandler::fromSDL(std::string_view context) {
    const char* sdlError = SDL_GetError();
    if (!sdlError || sdlError[0] == '\0') {
        sdlError = "Unknown SDL error";
    }

    std::string message = std::string("SDL Error: ") + sdlError;
    return EngineError(EngineError::Category::SDL, message, context);
}

EngineError ErrorHandler::fromGPU(std::string_view context) {
    // SDL_GPU doesn't have detailed error messages like Vulkan
    // We can add more context based on the operation
    std::string message = "GPU operation failed";
    return EngineError(EngineError::Category::GPU, message, context);
}

bool ErrorHandler::canRecover(const EngineError& error) {
    switch (error.category) {
        case EngineError::Category::Asset:
            // Asset loading failures are usually recoverable
            return true;
        case EngineError::Category::FileSystem:
            // File not found might be recoverable
            return true;
        case EngineError::Category::SDL:
        case EngineError::Category::GPU:
            // System-level errors usually not recoverable
            return false;
        default:
            return false;
    }
}

void ErrorHandler::attemptRecovery(const EngineError& error) {
    switch (error.category) {
        case EngineError::Category::Asset:
            logWarning("Attempting to use fallback asset for: " + error.context);
            // Could load a default texture/material here
            break;
        case EngineError::Category::FileSystem:
            logWarning("File access failed, checking alternative paths: " + error.context);
            // Could try different file paths
            break;
        default:
            logError(error);
            break;
    }
}

} // namespace Astral