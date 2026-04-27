#include "editor/panels/content_browser_panel.h"
#include <imgui.h>
#include <SDL3/SDL.h>

ContentBrowserPanel::ContentBrowserPanel() {
    m_assetsDirectory = "assets";
    m_currentDirectory = m_assetsDirectory;
    
    // Klasör yoksa oluştur
    if (!std::filesystem::exists(m_assetsDirectory)) {
        std::filesystem::create_directory(m_assetsDirectory);
    }
}

void ContentBrowserPanel::draw() {
    ImGui::Begin(getName());

    if (m_currentDirectory != m_assetsDirectory) {
        if (ImGui::Button("<-")) {
            m_currentDirectory = m_currentDirectory.parent_path();
        }
    }

    ImGui::SameLine();
    ImGui::Text("Path: %s", m_currentDirectory.string().c_str());

    ImGui::Separator();

    static float padding = 16.0f;
    static float thumbnailSize = 64.0f;
    float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentDirectory)) {
        const auto& path = directoryEntry.path();
        auto relativePath = std::filesystem::relative(path, m_assetsDirectory);
        std::string filenameString = path.filename().string();

        ImGui::PushID(filenameString.c_str());
        
        // Klasör mü dosya mı?
        if (directoryEntry.is_directory()) {
            if (ImGui::Button("Folder", ImVec2(thumbnailSize, thumbnailSize))) {
                m_currentDirectory /= path.filename();
            }
        } else {
            // Dosya ikonları/butonları
            std::string ext = path.extension().string();
            const char* icon = "File";
            if (ext == ".prefab") icon = "Prefab";
            else if (ext == ".astral") icon = "Scene";
            else if (ext == ".png" || ext == ".jpg") icon = "Texture";
            
            ImGui::Button(icon, ImVec2(thumbnailSize, thumbnailSize));
            
            // Drag and Drop (Prefab için)
            if (ext == ".prefab" && ImGui::BeginDragDropSource()) {
                std::string pathStr = path.string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
                ImGui::TextUnformatted(filenameString.c_str());
                ImGui::EndDragDropSource();
            }
        }

        ImGui::TextWrapped("%s", filenameString.c_str());
        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    ImGui::End();
}
