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
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_assetsDirectory;

    // Rename state
    bool m_isRenaming{ false };
    std::filesystem::path m_renamePath;
    char m_renameBuffer[256] = "";
};
