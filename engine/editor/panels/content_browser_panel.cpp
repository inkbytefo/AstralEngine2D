#include "editor/panels/content_browser_panel.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>

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

    // Browser Grid — BeginTable API (deprecated Columns yerine)
    static float padding = 16.0f;
    static float thumbnailSize = 72.0f;
    float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    if (ImGui::BeginTable("ContentBrowserGrid", columnCount, ImGuiTableFlags_NoBordersInBody))
    {
        for (auto& directoryEntry : std::filesystem::directory_iterator(m_currentDirectory)) {
            const auto& path = directoryEntry.path();
            std::string filenameString = path.filename().string();
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            ImGui::TableNextColumn();
            ImGui::PushID(filenameString.c_str());
            
            // Dosya tipi icon ve rengi
            const char* icon = "File";
            ImVec4 iconColor = ImVec4(1, 1, 1, 1);

            if (directoryEntry.is_directory()) {
                icon = "Folder";
                iconColor = ImVec4(0.9f, 0.7f, 0.1f, 1.0f);
            } else {
                if (extension == ".prefab")                        { icon = "Prefab";  iconColor = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); }
                else if (extension == ".astral")                   { icon = "Scene";   iconColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); }
                else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp")
                                                                   { icon = "Texture"; iconColor = ImVec4(0.8f, 0.4f, 0.8f, 1.0f); }
                else if (extension == ".gltf" || extension == ".glb" || extension == ".obj" || extension == ".fbx")
                                                                   { icon = "Model";   iconColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); }
                else if (extension == ".vert" || extension == ".frag" || extension == ".glsl" || extension == ".hlsl")
                                                                   { icon = "Shader";  iconColor = ImVec4(1.0f, 0.5f, 0.5f, 1.0f); }
                else if (extension == ".json" || extension == ".xml" || extension == ".yaml")
                                                                   { icon = "Config";  iconColor = ImVec4(0.6f, 0.6f, 0.9f, 1.0f); }
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
            if (ImGui::Button(icon, ImVec2(thumbnailSize, thumbnailSize))) {
                // Tek tıklama — gerekirse seçim mantığı
            }
            ImGui::PopStyleColor(3);

            // Çift tıklama — klasörü aç veya dosya işlemi
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (directoryEntry.is_directory()) {
                    m_currentDirectory /= path.filename();
                } else {
                    SDL_Log("Opening file: %s", filenameString.c_str());
                }
            }

            // Drag and Drop Source
            if (ImGui::BeginDragDropSource()) {
                std::string pathStr = path.string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
                ImGui::TextUnformatted(filenameString.c_str());
                ImGui::EndDragDropSource();
            }

            // Context Menu (Rename + Delete)
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Rename")) {
                    m_isRenaming = true;
                    m_renamePath = path;
                    strncpy(m_renameBuffer, path.filename().string().c_str(), sizeof(m_renameBuffer) - 1);
                    m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
                }
                if (ImGui::MenuItem("Delete")) {
                    std::filesystem::remove_all(path);
                }
                ImGui::EndPopup();
            }

            // Dosya adı (rename modu kontrol)
            if (m_isRenaming && m_renamePath == path)
            {
                ImGui::PushItemWidth(thumbnailSize);
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    // Enter — yeniden adlandır
                    std::filesystem::path newPath = path.parent_path() / m_renameBuffer;
                    if (m_renameBuffer[0] != '\0' && newPath != path)
                    {
                        try {
                            std::filesystem::rename(path, newPath);
                        } catch (const std::exception& e) {
                            SDL_Log("Rename failed: %s", e.what());
                        }
                    }
                    m_isRenaming = false;
                }
                ImGui::PopItemWidth();

                // Escape veya başka yere tıklama ile iptal
                if (ImGui::IsKeyPressed(ImGuiKey_Escape) || 
                    (!ImGui::IsItemActive() && !ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)))
                {
                    m_isRenaming = false;
                }
            }
            else
            {
                ImGui::TextWrapped("%s", filenameString.c_str());
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
