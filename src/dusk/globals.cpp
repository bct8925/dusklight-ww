#include <dolphin/types.h>
#include "dusk/dusk.h"
#include "dusk/main.h"

bool dusk::IsRunning = true;
bool dusk::IsShuttingDown = false;
bool dusk::IsGameLaunched = false;
bool dusk::RestartRequested = false;
uint8_t dusk::SaveRequested = 0;
dusk::StageRequest dusk::StageRequested{"", false};
std::filesystem::path dusk::ConfigPath;
std::filesystem::path dusk::CachePath;
AuroraStats dusk::lastFrameAuroraStats;
float dusk::frameUsagePct = 0.0f;

void dusk::RequestRestart() noexcept {
    RestartRequested = SupportsProcessRestart;
    IsRunning = false;
}

// dusklight also defined Twilight Princess and SDK globals here (g_kankyoHIO, DSP task
// pointers, ...). Wind Waker defines the ones it uses itself (e.g. dDebugPad in d_debug_pad.cpp).
