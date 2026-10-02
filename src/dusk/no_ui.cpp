// Stand-ins for the RmlUi/imgui shell pieces while DUSK_GAME_WW builds without them
// (DUSK_TP_FEATURE_FILES). Remove from the build once the UI is ported.

#include "dusk/logging.h"
#include "dusk/ui/ui.hpp"

namespace {
// The stub log window lives in imgui. Without it, leave stub messages in the normal log.
const bool s_stubLogDisabled = (StubLogEnabled = false, true);
}  // namespace

void dusk::SendToStubLog(borealis::LogLevel, std::string_view, std::string_view) {}

bool dusk::ui::any_document_visible() noexcept {
    return false;
}

void dusk::ui::apply_scale() noexcept {}
