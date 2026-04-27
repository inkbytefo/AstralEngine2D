#pragma once
#include <stdexcept>
#include <string>
#include <string_view>

namespace Astral {

/**
 * @brief Engine-wide error handling system
 * Provides structured error reporting with categories and context
 */
class EngineError : public std::runtime_error {
public:
    enum class Category {
        SDL,           // SDL-related errors
        GPU,           // GPU/Graphics device errors
        Asset,         // Asset loading/management errors
        ECS,           // Entity-Component-System errors
        Rendering,     // Rendering pipeline errors
        FileSystem,    // File I/O errors
        Memory,        // Memory allocation/management errors
        Configuration  // Configuration/setup errors
    };

    EngineError(Category cat, std::string_view message,
                std::string_view context = "")
        : std::runtime_error(std::string(message))
        , category(cat)
        , context(context) {}

    Category category;
    std::string context;
};

/**
 * @brief Error handling utilities
 */
class ErrorHandler {
public:
    static void logError(const EngineError& error);
    static void logWarning(std::string_view message);
    static void logInfo(std::string_view message);

    // Convert SDL errors to EngineError
    static EngineError fromSDL(std::string_view context = "");
    static EngineError fromGPU(std::string_view context = "");

    // Error recovery strategies
    static bool canRecover(const EngineError& error);
    static void attemptRecovery(const EngineError& error);
};

// Convenience macros for error throwing
#define ENGINE_ERROR(category, message) \
    throw Astral::EngineError(Astral::EngineError::Category::category, message, __FUNCTION__)

#define ENGINE_ERROR_CTX(category, message, context) \
    throw Astral::EngineError(Astral::EngineError::Category::category, message, context)

#define SDL_ERROR_CHECK(operation) \
    do { \
        if (!(operation)) { \
            throw Astral::ErrorHandler::fromSDL(#operation); \
        } \
    } while(0)

#define GPU_ERROR_CHECK(operation) \
    do { \
        if (!(operation)) { \
            throw Astral::ErrorHandler::fromGPU(#operation); \
        } \
    } while(0)

} // namespace Astral