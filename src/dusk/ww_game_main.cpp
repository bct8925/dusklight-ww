// game_main for Wind Waker: what src/dusk/main.cpp calls after platform setup. Sets up settings,
// logging and aurora, opens the disc, then runs the game (mDoMain_run in m_Do_main.cpp).
// Adapted from dusklight's m_Do_main.cpp, without the UI, mods, Discord or crash reporting,
// which are not ported to Wind Waker yet.

#include "dusk/app_info.hpp"
#include "dusk/config.hpp"
#include "dusk/data.hpp"
#include "dusk/dusk.h"
#include "dusk/logging.h"
#include "dusk/main.h"
#include "dusk/settings.h"
#include "dusk/version.hpp"

#include "m_Do/m_Do_dvd_thread.h"
#include "m_Do/m_Do_main.h"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/event.h>
#include <borealis/aurora_log.h>
#include <borealis/cli.hpp>
#include <borealis/crash.hpp>
#include <borealis/io.hpp>
#include <borealis/version.h>
#include <cxxopts.hpp>
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <fmt/format.h>

#include <cstdio>
#include <string>
#include <string_view>

int mDoMain_run(int argc, const char* argv[]);

namespace {

// The GameCube renders 640x480; open the window at twice that.
constexpr int kDefaultWindowWidth = 640 * 2;
constexpr int kDefaultWindowHeight = 480 * 2;

bool try_parse_backend(std::string_view name, AuroraBackend& out) {
    static constexpr std::pair<std::string_view, AuroraBackend> kBackends[] = {
        {"auto", BACKEND_AUTO},     {"d3d12", BACKEND_D3D12},   {"d3d11", BACKEND_D3D11},
        {"metal", BACKEND_METAL},   {"vulkan", BACKEND_VULKAN}, {"opengl", BACKEND_OPENGL},
        {"opengles", BACKEND_OPENGLES}, {"webgpu", BACKEND_WEBGPU}, {"null", BACKEND_NULL},
    };
    for (const auto& [id, backend] : kBackends) {
        if (name == id) {
            out = backend;
            return true;
        }
    }
    return false;
}

AuroraBackend resolve_backend(const cxxopts::ParseResult& args) {
    AuroraBackend backend = BACKEND_AUTO;
    std::string requested = args.count("backend") ? args["backend"].as<std::string>()
                                                  : static_cast<const std::string&>(
                                                        dusk::getSettings().backend.graphicsBackend);
    if (!requested.empty() && !try_parse_backend(requested, backend)) {
        DuskLog.warn("Unknown graphics backend '{}', using auto", requested);
        backend = BACKEND_AUTO;
    }
    return backend;
}

}  // namespace

int game_main(int argc, char* argv[]) {
    cxxopts::ParseResult args;
    borealis::cli::StandardOptions standardOptions;
    try {
        cxxopts::Options options("Dusklight-WW", "PC port of The Legend of Zelda: The Wind Waker");
        borealis::cli::add_standard_options(options);
        options.add_options()
            ("h,help", "Print usage")
            ("dvd", "Path to the disc image (GZLE01)", cxxopts::value<std::string>())
            ("backend", "Graphics backend (auto, d3d12, d3d11, metal, vulkan, null)",
             cxxopts::value<std::string>())
            ("develop", "Enable the game's development mode and OSReport output",
             cxxopts::value<bool>()->default_value("false")->implicit_value("true"));
        options.parse_positional({"dvd"});
        options.positional_help("<dvd-image>");
        options.allow_unrecognised_options();
        args = options.parse(argc, argv);
        standardOptions = borealis::cli::parse(args);
        if (args.count("help")) {
            std::printf("%s\n", options.help().c_str());
            return 0;
        }
    } catch (const cxxopts::exceptions::exception& e) {
        std::fprintf(stderr, "Argument error: %s\n", e.what());
        return 1;
    }

    dusk::registerSettings();
    const auto dataPaths = dusk::data::initialize_data(standardOptions.userDir);
    dusk::ConfigPath = dataPaths.userPath;
    dusk::CachePath = dataPaths.cachePath;
    dusk::InitializeLogging(dusk::CachePath, standardOptions);
    DuskLog.info("Dusklight-WW {} (rev {}, built {}, {})", BOREALIS_APP_DESCRIBE,
                 BOREALIS_APP_REVISION, BOREALIS_APP_DATE, BOREALIS_BUILD_TYPE);
    dusk::config::load_from_user_preferences();
    borealis::crash::install();

    {
        const auto userPath = dusk::ConfigPath.u8string();
        const auto cachePath = dusk::CachePath.u8string();
        AuroraConfig config{};
        config.appName = dusk::AppName;
        config.userPath = reinterpret_cast<const char*>(userPath.c_str());
        config.cachePath = reinterpret_cast<const char*>(cachePath.c_str());
        config.vsync = dusk::getSettings().video.enableVsync;
        config.startFullscreen = dusk::getSettings().video.enableFullscreen;
        config.windowPosX = -1;
        config.windowPosY = -1;
        config.windowWidth = kDefaultWindowWidth;
        config.windowHeight = kDefaultWindowHeight;
        config.desiredBackend = resolve_backend(args);
        config.logCallback = borealis::log::aurora_callback();
        config.logLevel = borealis::log::to_aurora_level(borealis::log::level());
        config.mem1Size = 256 * 1024 * 1024;
        config.mem2Size = 24 * 1024 * 1024;
        config.allowJoystickBackgroundEvents = dusk::getSettings().game.allowBackgroundInput;
        config.pauseOnFocusLost = dusk::getSettings().game.pauseOnFocusLost;
        config.allowTextureDumps = false;
        const AuroraInfo info = aurora_initialize(argc, argv, &config);
        if (info.backend == BACKEND_NULL) {
            DuskLog.fatal("No graphics backend could be initialized");
        }
    }
    VISetWindowTitle(fmt::format("Dusklight-WW {}", BOREALIS_APP_DESCRIBE).c_str());

    std::string dvdLocation = args.count("dvd") ? args["dvd"].as<std::string>()
                                                : static_cast<const std::string&>(
                                                      dusk::getSettings().backend.isoPath);
    if (dvdLocation.empty()) {
        DuskLog.fatal("No disc image: pass the path to your GZLE01 image, e.g. --dvd game.iso");
    }
    const auto dvdAccess = borealis::io::access_path(dvdLocation);
    const std::string dvdPath =
        dvdAccess ? borealis::io::fs_path_to_string(dvdAccess.path()) : dvdLocation;
    DuskLog.info("Loading disc image: {}", dvdPath);
    if (!aurora_dvd_open(dvdPath.c_str())) {
        DuskLog.fatal("Failed to open disc image: {}", dvdPath);
    }
    dusk::getSettings().backend.isoPath.setValue(dvdLocation);
    dusk::config::save();
    dusk::IsGameLaunched = true;

    if (args.count("develop") && args["develop"].as<bool>()) {
        mDoMain::developmentMode = 1;
    }

    dusk::version::init();
    OSInit();
    mDoDvdThd::SyncWidthSound = false;

    const int result = mDoMain_run(argc, const_cast<const char**>(argv));

    // Wakes any threads still waiting on OS primitives so they can exit.
    OSResetSystem(OS_RESET_SHUTDOWN, 0, 0);
    borealis::log::shutdown();
    dusk::config::shutdown();
    aurora_shutdown();
    return result;
}
