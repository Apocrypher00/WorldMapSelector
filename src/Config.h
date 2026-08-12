#pragma once

namespace WMS::Config
{
    void Load();
    std::uint32_t GetOpenSelectorKey();
    bool GetOpenMapAfterSelection();
    bool GetPersistSelection();
    bool GetAllowChooserOutsideMap();
    bool GetAllowChooserWhileMapOpen();
    bool GetShowMapMenuKeyHint();
    bool IsWorldspaceIncluded(std::string_view editorID);
    bool IsWorldspaceExcluded(std::string_view editorID);
}
