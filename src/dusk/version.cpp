#include "dusk/version.hpp"

#include "dusk/logging.h"

namespace dusk::version {

using namespace std::string_view_literals;

static bool versionInitialized;
static GameVersion gameVersion;
static DVDDiskID diskId;

void init() {
    versionInitialized = true;

    if (!DVDLowReadDiskID(&diskId, nullptr)) {
        DuskLog.fatal("DVDLowReadDiskID failed to return instantly.");
    }

    std::string_view company(diskId.company, sizeof(diskId.company));
    std::string_view game(diskId.gameName, sizeof(diskId.gameName));

    if (company != "01"sv) {
        DuskLog.fatal("Wrong company ID in disc: {}", company);
    }

    if (game == "GZLE"sv && diskId.gameVersion == 0) {
        gameVersion = GameVersion::GcnUsa;
    } else if (game == "GZLE"sv && diskId.gameVersion == 48) {
        gameVersion = GameVersion::GcnKor;
    } else if (game == "GZLP"sv) {
        gameVersion = GameVersion::GcnPal;
    } else if (game == "GZLJ"sv) {
        gameVersion = GameVersion::GcnJpn;
    } else {
        DuskLog.fatal("Not a Wind Waker disc, or an unknown revision: {}{} rev {}", game, company,
                      diskId.gameVersion);
    }

    // The game code is built for the USA release only (VERSION=2). Other releases differ in code
    // as well as data, so they are refused until they are supported.
    if (gameVersion != GameVersion::GcnUsa) {
        DuskLog.fatal("Only the USA release of The Wind Waker (GZLE01, revision 0) is supported "
                      "so far; this disc is {}{} rev {}", game, company, diskId.gameVersion);
    }

    DuskLog.info("Loaded game disc is {}{} rev {}", game, company, diskId.gameVersion);
}

bool isRegionJpn() {
    return getGameVersion() == GameVersion::GcnJpn;
}

bool isRegionPal() {
    return getGameVersion() == GameVersion::GcnPal;
}

bool isRegionUsa() {
    return getGameVersion() == GameVersion::GcnUsa || getGameVersion() == GameVersion::GcnKor;
}

GameVersion getGameVersion() {
    if (!versionInitialized) {
        abort();
    }

    return gameVersion;
}

const DVDDiskID& getDiskID() {
    return diskId;
}

}  // namespace dusk::version
