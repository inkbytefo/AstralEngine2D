#include "editor/panels/console_panel.h"
#include <imgui.h>

std::vector<ConsolePanel::LogEntry> ConsolePanel::s_logs;
std::mutex ConsolePanel::s_logMutex;

ConsolePanel::ConsolePanel() {
    SDL_SetLogOutputFunction(logOutputFunction, this);
}

ConsolePanel::~ConsolePanel() {
    SDL_SetLogOutputFunction(nullptr, nullptr);
}

void ConsolePanel::logOutputFunction(void* userdata, int category, SDL_LogPriority priority, const char* message) {
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_logs.push_back({ message, priority });
}

void ConsolePanel::draw() {
    ImGui::Begin(getName());

    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lock(s_logMutex);
        s_logs.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);

    ImGui::Separator();

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(s_logMutex);
    for (const auto& log : s_logs) {
        ImVec4 color = ImVec4(1, 1, 1, 1);
        if (log.priority == SDL_LOG_PRIORITY_WARN) color = ImVec4(1, 1, 0, 1);
        else if (log.priority == SDL_LOG_PRIORITY_ERROR) color = ImVec4(1, 0, 0, 1);
        else if (log.priority == SDL_LOG_PRIORITY_DEBUG) color = ImVec4(0, 1, 1, 1);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(log.message.c_str());
        ImGui::PopStyleColor();
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}
