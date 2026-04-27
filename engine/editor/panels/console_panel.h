#pragma once

#include "editor/panels/editor_panel.h"
#include <vector>
#include <string>
#include <mutex>
#include <SDL3/SDL.h>

class ConsolePanel : public IEditorPanel {
public:
    ConsolePanel();
    ~ConsolePanel();

    void draw() override;
    const char* getName() const override { return "Console"; }

    static void addLog(const std::string& log);

private:
    struct LogEntry {
        std::string message;
        SDL_LogPriority priority;
    };

    static void logOutputFunction(void* userdata, int category, SDL_LogPriority priority, const char* message);

private:
    static std::vector<LogEntry> s_logs;
    static std::mutex s_logMutex;
    bool m_autoScroll{ true };
};
