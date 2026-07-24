#include "PCH.h"

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    SKSE::log::info("WorldMapSelector loaded successfully.");
    return true;
}
