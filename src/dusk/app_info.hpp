#pragma once

#include <borealis/app_info.hpp>

namespace dusk {
    /** Application identity fields for Borealis modules */
    // Wind Waker port: its own identity, so it never shares (or overwrites) a Dusklight
    // installation's settings, saves or cache.
    inline constexpr borealis::AppInfo AppInfo{
        .orgName = "bct8925",
        .appName = "Dusklight-WW",
        .githubOwner = "bct8925",
        .githubRepo = "dusklight-ww",
        .discordApplicationId = "",
    };

    /**
     * \brief The internal application name for the game.
     *
     * This gets used for file paths and such, and cannot be changed!
     */
    constexpr auto AppName = "Dusklight-WW";

}
