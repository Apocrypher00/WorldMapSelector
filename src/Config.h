#pragma once

namespace WMS::Config
{
    void Load();
    std::uint32_t GetOpenSelectorKey();
    bool GetOpenMapAfterSelection();
    bool GetPersistSelection();
    bool GetAllowChooserWhileMapOpen();
    bool IsWorldspaceExcluded(std::string_view editorID);
}
