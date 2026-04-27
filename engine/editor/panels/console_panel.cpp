#include "editor/panels/console_panel.h"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

std::vector<ConsolePanel::LogEntry> ConsolePanel::s_logs;
std::mutex ConsolePanel::s_logMutex;

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    return ss.str();
}

ConsolePanel::ConsolePanel() {
    SDL_SetLogOutputFunction(logOutputFunction, this);
}

ConsolePanel::~ConsolePanel() {
    SDL_SetLogOutputFunction(nullptr, nullptr);
}

void ConsolePanel::logOutputFunction(void* userdata, int category, SDL_LogPriority priority, const char* message) {
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_logs.push_back({ message, priority, getTimestamp() });
}

void ConsolePanel::addLog(const std::string& log, SDL_LogPriority priority) {
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_logs.push_back({ log, priority, getTimestamp() });
}

void ConsolePanel::draw() {
    ImGui::Begin(getName());

    // Toolbar
    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lock(s_logMutex);
        s_logs.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    
    // Search Filter
    ImGui::Text("Filter:");
    ImGui::SameLine();
    ImGui::PushItemWidth(150);
    ImGui::InputText("##ConsoleFilter", m_filterBuffer, sizeof(m_filterBuffer));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Priority Filters
    m_infoCount = 0; m_warnCount = 0; m_errorCount = 0;
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        for (const auto& log : s_logs) {
            if (log.priority == SDL_LOG_PRIORITY_WARN) m_warnCount++;
            else if (log.priority >= SDL_LOG_PRIORITY_ERROR) m_errorCount++;
            else m_infoCount++;
        }
    }

    auto drawFilterToggle = [&](const char* label, bool& val, int count, ImVec4 color) {
        ImGui::PushStyleColor(ImGuiCol_Text, val ? color : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        std::string btnLabel = std::string(label) + " (" + std::to_string(count) + ")";
        if (ImGui::Selectable(btnLabel.c_str(), &val, 0, ImVec2(ImGui::CalcTextSize(btnLabel.c_str()).x, 0))) {}
        ImGui::PopStyleColor();
        ImGui::SameLine();
    };

    drawFilterToggle("Info", m_showInfo, m_infoCount, ImVec4(1, 1, 1, 1));
    drawFilterToggle("Warn", m_showWarn, m_warnCount, ImVec4(1, 1, 0, 1));
    drawFilterToggle("Error", m_showError, m_errorCount, ImVec4(1, 0.2f, 0.2f, 1.0f));

    ImGui::Separator();

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(s_logMutex);
    std::string searchStr = m_filterBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    for (const auto& log : s_logs) {
        // Apply Filters
        if (log.priority == SDL_LOG_PRIORITY_WARN && !m_showWarn) continue;
        if (log.priority >= SDL_LOG_PRIORITY_ERROR && !m_showError) continue;
        if (log.priority < SDL_LOG_PRIORITY_WARN && !m_showInfo) continue;

        if (!searchStr.empty()) {
            std::string msg = log.message;
            std::transform(msg.begin(), msg.end(), msg.begin(), ::tolower);
            if (msg.find(searchStr) == std::string::npos) continue;
        }

        ImVec4 color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        if (log.priority == SDL_LOG_PRIORITY_WARN) color = ImVec4(1, 1, 0, 1);
        else if (log.priority >= SDL_LOG_PRIORITY_ERROR) color = ImVec4(1, 0.2f, 0.2f, 1.0f);
        else if (log.priority == SDL_LOG_PRIORITY_DEBUG) color = ImVec4(0, 1, 1, 1);

        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%s]", log.timestamp.c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(log.message.c_str());
        ImGui::PopStyleColor();
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}
