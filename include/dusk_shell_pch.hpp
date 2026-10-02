#pragma once

// Precompiled header for the port layer (dusk_internal). dusklight's shell code assumes the
// game PCH has already pulled in global.h and the Dolphin SDK; this provides just those, so the
// shell builds without the game tree.
#include "global.h"
#include <dolphin/dolphin.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <string_view>
