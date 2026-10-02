# The game ABI surface shared by the main build and the mod SDK (sdk/CMakeLists.txt)
include_guard(GLOBAL)

get_filename_component(_game_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# VERSION=2 is VERSION_USA (GZLE01) in TWW's global.h; 0 would select the demo build.
# PARTIAL_DEBUG makes debug and release share one struct/vtable ABI so a mod binary loads into either
set(_game_compile_defs TARGET_PC=1 WIDESCREEN_SUPPORT=1 AVOID_UB=1 VERSION=2 MTX_USE_PS=1 PARTIAL_DEBUG=1)
if (ANDROID)
    list(APPEND _game_compile_defs TARGET_ANDROID=1)
endif ()

# Public game headers
set(_game_abi_include_dirs
        ${_game_root}/include
        ${_game_root}/assets/GZLE01
        ${_game_root}/extern/aurora/include/dolphin
        ${_game_root}/extern/aurora/include
        ${_game_root}/sdk_compat
        ${_game_root}/sdk/include
)

# Internal game headers
set(_game_include_dirs
        ${_game_abi_include_dirs}
        ${_game_root}/src
        ${_game_root}/extern
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Mod API, including services
add_library(dusklight_mod_api INTERFACE)
target_include_directories(dusklight_mod_api INTERFACE ${_game_root}/sdk/include)

# Full internal headers used to build the game
add_library(dusklight_game_headers INTERFACE)
target_include_directories(dusklight_game_headers INTERFACE ${_game_include_dirs})
target_compile_definitions(dusklight_game_headers INTERFACE ${_game_compile_defs})
# Data the decomp embeds in the DOL (display lists, textures, fonts) is #included from headers
# its own build extracts from the disc into build/GZLE01/include/assets. Until that data is read
# from the disc at runtime, point this at a local tww build. Never commit or distribute it: a
# binary built this way contains Nintendo data.
set(WW_LOCAL_ASSETS "${_game_root}/../tww/build/GZLE01/include" CACHE PATH
        "Include dir of a local zeldaret/tww build (contains assets/*.h extracted from the disc)")
if (EXISTS "${WW_LOCAL_ASSETS}/assets")
    target_include_directories(dusklight_game_headers INTERFACE "${WW_LOCAL_ASSETS}")
else ()
    message(WARNING "WW_LOCAL_ASSETS (${WW_LOCAL_ASSETS}) has no assets/; files that embed DOL data will not compile")
endif ()

if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # MWCC (and GCC/Clang, with a warning) allow passing string literals as char*; the decomp
    # relies on it throughout.
    target_compile_options(dusklight_game_headers INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:/Zc:strictStrings->")
endif ()

# Decomp sources rely on global.h and on the decomp SDK's extra types (uint, Mtx33, SVec...) being
# visible even when they include dolphin/types.h directly, as MWCC's prefix setup made them.
# global.h pulls in sdk_compat/pc_sdk_extras.h, so force-include it everywhere.
if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(dusklight_game_headers INTERFACE "/FIglobal.h")
else ()
    target_compile_options(dusklight_game_headers INTERFACE "SHELL:-include global.h")
endif ()

# Public game ABI for mods
add_library(dusklight_game_abi_headers INTERFACE)
target_include_directories(dusklight_game_abi_headers INTERFACE ${_game_abi_include_dirs})
target_compile_definitions(dusklight_game_abi_headers INTERFACE ${_game_compile_defs})

# Mod feature targets
add_library(dusklight_mod_feature_game INTERFACE)
target_link_libraries(dusklight_mod_feature_game INTERFACE
        dusklight_mod_api
        dusklight_game_abi_headers)
target_compile_definitions(dusklight_mod_feature_game INTERFACE DUSK_MOD_FEATURE_GAME=1)
# Game headers assume global.h comes first in the translation unit (it defines DUSK_GAME_DATA
# and friends); force-include it so mods don't depend on include order.
if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(dusklight_mod_feature_game INTERFACE
            "$<$<COMPILE_LANGUAGE:CXX>:/FIglobal.h>")
else ()
    target_compile_options(dusklight_mod_feature_game INTERFACE
            "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include global.h>")
endif ()
target_sources(dusklight_mod_feature_game INTERFACE
        ${_game_root}/sdk/src/game_feature.cpp)

add_library(dusklight_mod_feature_webgpu INTERFACE)
target_link_libraries(dusklight_mod_feature_webgpu INTERFACE dusklight_mod_api)
target_compile_definitions(dusklight_mod_feature_webgpu INTERFACE DUSK_MOD_FEATURE_WEBGPU=1)

add_library(dusklight_mod_feature_fmt INTERFACE)
target_link_libraries(dusklight_mod_feature_fmt INTERFACE dusklight_mod_api)
target_compile_definitions(dusklight_mod_feature_fmt INTERFACE DUSK_MOD_FEATURE_FMT=1)
