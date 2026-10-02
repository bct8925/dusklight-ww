#pragma once

/**
 * Functionality for switching game behavior based on the loaded game version (e.g. USA/PAL/JPN)
 */
namespace dusk::version {
    enum class GameVersion : u8 {
        GcnUsa,  // GZLE01 revision 0
        GcnKor,  // GZLE01 revision 48
        GcnPal,  // GZLP01
        GcnJpn,  // GZLJ01
    };

    bool isRegionPal();
    bool isRegionJpn();
    bool isRegionUsa();

    GameVersion getGameVersion();

    const DVDDiskID& getDiskID();

    void init();

    template<typename T>
    struct VersionOption {
        GameVersion mVersion;
        T mValue;

        constexpr VersionOption(GameVersion version, T value) : mVersion(version), mValue(value) {}
    };

    template<typename T>
    const T& versionSelect(const std::initializer_list<VersionOption<T>> options) {
        const auto version = getGameVersion();
        for (const auto& opt : options) {
            if (opt.mVersion == version) {
                return opt.mValue;
            }
        }

        // Unable to find value.
        abort();
    }

    template<typename T>
    const T& versionSelect(const std::initializer_list<VersionOption<T>> options, const T& defaultValue) {
        const auto version = getGameVersion();
        for (const auto& opt : options) {
            if (opt.mVersion == version) {
                return opt.mValue;
            }
        }

        return defaultValue;
    }

    template<typename T>
    T regionSelect(const T& usa, const T& pal, const T& jpn) {
        return isRegionUsa() ? usa : isRegionPal() ? pal : jpn;
    }
}  // namespace dusk::version
