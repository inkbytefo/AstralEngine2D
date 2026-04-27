#pragma once

#include "editor/panels/editor_panel.h"
#include <filesystem>
#include <string>

class ContentBrowserPanel : public IEditorPanel {
public:
    ContentBrowserPanel();
    void draw() override;
    const char* getName() const override { return "Content Browser"; }

private:
    void drawDirectory(const std::filesystem::path& path);

private:
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_assetsDirectory;
};
