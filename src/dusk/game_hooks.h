#pragma once

// The game-specific pieces the shell (UI, settings) relies on, kept in one place so the rest of
// src/dusk does not include game code. These are the Wind Waker definitions.

#include <dolphin/types.h>

#include "JAZelAudio/JAZelAudio_SE.h"

namespace dusk::game {

// Plays a menu sound effect, by the game's own sound effect ID.
void play_menu_sound(u32 soundId);

// UI sound effects. Picked from Wind Waker's menu sounds by name; adjust by ear once audio works.

// Button clicked/pressed
constexpr u32 kSoundClick = JA_SE_ITM_MENU_DECIDE;
// "Play" button clicked/pressed
constexpr u32 kSoundPlay = JA_SE_OK_1;
// Input binding changed
constexpr u32 kSoundBindingChanged = JA_SE_ITM_MENU_SET;

// Menu button pressed (open/close menu bar or hide/show the active window)
constexpr u32 kSoundMenuOpen = JA_SE_ITM_SUBMENU_IN_1;
constexpr u32 kSoundMenuClose = JA_SE_ITM_SUBMENU_OUT;

// Window opened/closed
constexpr u32 kSoundWindowOpen = JA_SE_ITM_MENU_IN;
constexpr u32 kSoundWindowClose = JA_SE_ITM_MENU_OUT;

// Window tab changed
constexpr u32 kSoundTabChanged = JA_SE_ITM_MENU_PAGE;

// Item within menu focused
constexpr u32 kSoundItemFocus = JA_SE_ITM_MENU_CURSOR;
// Item changed (e.g. number input left/right)
constexpr u32 kSoundItemChange = JA_SE_CURSOR_MOVE_1;
// Item enabled ("On") / disabled ("Off")
constexpr u32 kSoundItemEnable = JA_SE_ITM_MENU_OPT_SW;
constexpr u32 kSoundItemDisable = JA_SE_ITM_MENU_OPT_SW;

// Achievement unlocked
constexpr u32 kSoundAchievementUnlock = JA_SE_OK_1;
// Warning shown
constexpr u32 kSoundWarning = JA_SE_CANCEL_1;

}  // namespace dusk::game
