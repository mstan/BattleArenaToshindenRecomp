#include "mod_packages.h"

#include <filesystem>
#include <iostream>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;
using namespace PSXRecompV4;

static int failures;

static void check(bool value, const std::string& message) {
    if (!value) {
        std::cerr << "FAIL: " << message << "\n";
        failures++;
    }
}

static void check_package_resolves_without_exe_hash(const fs::path& mods_root,
                                                    const std::string& package_id,
                                                    const std::string& feature_id) {
    ModPackageManager manager(mods_root);
    std::string error;
    check(manager.scan(&error), error);
    check(manager.set_feature_enabled(package_id, feature_id, true, &error), error);

    ModResolution resolution = manager.resolve("SCUS-94200", "", "");
    const bool target_rejected =
        std::any_of(resolution.errors.begin(), resolution.errors.end(),
                    [&](const std::string& resolution_error) {
                        return resolution_error ==
                            "package does not target this game/image: " + package_id;
                    });
    check(!target_rejected, package_id + " must target clean player installs");
    const auto requires_hook = [&](const char* id) {
        check(std::any_of(resolution.plugins.begin(), resolution.plugins.end(),
                          [&](const auto& plugin) { return plugin.id == id; }),
              package_id + " must explicitly select hook " + id);
    };
    if (package_id == "toshinden.enhancement.widescreen") {
        requires_hook("toshinden.widescreen.final-ot");
        requires_hook("toshinden.widescreen.backdrop-prim");
    } else if (package_id == "toshinden.gameplay.boss-roster") {
        requires_hook("toshinden.boss-roster.select-helper");
        requires_hook("toshinden.boss-roster.ui-final-ot");
    }
}

int main() {
    const fs::path source_packages =
        fs::path(TOSHINDEN_SOURCE_ROOT) / "mods" / "preloaded";
    const fs::path test_root = fs::temp_directory_path() /
        "toshinden_preloaded_mod_targets_test";
    std::error_code ec;
    fs::remove_all(test_root, ec);
    fs::create_directories(test_root / "bundled", ec);
    if (ec) {
        std::cerr << "FAIL: cannot create temp bundled mod root: "
                  << ec.message() << "\n";
        return 1;
    }
    fs::copy(source_packages / "packages", test_root / "bundled",
             fs::copy_options::recursive, ec);
    if (ec) {
        std::cerr << "FAIL: cannot copy preloaded packages to temp root: "
                  << ec.message() << "\n";
        return 1;
    }

    check_package_resolves_without_exe_hash(
        test_root, "toshinden.enhancement.widescreen", "widescreen");
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.enhancement.frame-interpolation", "frame-interpolation");
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.gameplay.boss-roster", "boss-roster");
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.gameplay.desperation-at-any-health",
        "desperation-at-any-health");
    fs::remove_all(test_root, ec);
    return failures == 0 ? 0 : 1;
}
