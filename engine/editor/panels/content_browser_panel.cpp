#include "editor/panels/content_browser_panel.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <algorithm>

ContentBrowserPanel::ContentBrowserPanel() {
    m_assetsDirectory = "assets";
    m_currentDirectory = m_assetsDirectory;
    
    if (!std::filesystem::exists(m_assetsDirectory)) {
        std::filesystem::create_directory(m_assetsDirectory);
    }
}

void ContentBrowserPanel::draw() {
    ImGui::Begin(getName());

    // Navigation Bar
    if (m_currentDirectory != m_assetsDirectory) {
        if (ImGui::Button("<- Back")) {
            m_currentDirectory = m_currentDirectory.parent_path();
        }
        ImGui::SameLine();
    }

    ImGui::TextDisabled("Current Path: %s", m_currentDirectory.string().c_str());
    ImGui::Separator();

    // Browser Grid
    static float padding = 16.0f;
    static float thumbnailSize = 72.0f;
    float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentDirectory)) {
        const auto& path = directoryEntry.path();
        std::string filenameString = path.filename().string();
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        ImGui::PushID(filenameString.c_str());
        
        const char* icon = "File";
        ImVec4 iconColor = ImVec4(1, 1, 1, 1);

        if (directoryEntry.is_directory()) {
            icon = "Folder";
            iconColor = ImVec4(0.9f, 0.7f, 0.1f, 1.0f);
        } else {
            if (extension == ".prefab") { icon = "Prefab"; iconColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); }
            else if (extension == ".astral") { icon = "Scene"; iconColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); }
            else if (extension == ".png" || extension == ".jpg") { icon = "Texture"; iconColor = ImVec4(0.8f, 0.4f, 0.8f, 1.0f); }
            else if (extension == ".gltf" || extension == ".glb") { icon = "Model"; iconColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        if (ImGui::Button(icon, ImVec2(thumbnailSize, thumbnailSize))) {
            // Handle single click if needed
        }
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (directoryEntry.is_directory()) {
                m_currentDirectory /= path.filename();
            } else {
                // Open file action
                SDL_Log("Opening file: %s", filenameString.c_str());
            }
        }
        ImGui::PopStyleColor(2);

        // Drag and Drop
        if (ImGui::BeginDragDropSource()) {
            std::string pathStr = path.string();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
            ImGui::TextUnformatted(filenameString.c_str());
            ImGui::EndDragDropSource();
        }

        // Context Menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename")) { /* TODO */ }
            if (ImGui::MenuItem("Delete")) {
                std::filesystem::remove_all(path);
            }
            ImGui::EndPopup();
        }

        ImGui::TextWrapped("%s", filenameString.c_str());
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::End();
}
