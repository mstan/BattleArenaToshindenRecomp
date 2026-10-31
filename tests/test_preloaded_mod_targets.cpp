#include "mod_packages.h"

#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>

namespace fs = std::filesystem;
using namespace PSXRecompV4;

static int failures;

static void check(bool value, const std::string& message) {
    if (!value) {
        std::cerr << "FAIL: " << message << "\n";
        failures++;
    }
}

// This is a catalog/selection fixture, without guest execution. Supply the
// trusted IDs from src/mods constructors, as the runtime does before resolving
// a plan. Never derive registrations from the manifest being tested: that
// would hide a missing companion declaration. Callbacks must not run here.
static void catalog_callback() { std::abort(); }
static void catalog_entry(CPUState*, uint32_t) { std::abort(); }

static void register_catalog_fixture_plugins() {
    for (const char* id : {
             "toshinden.widescreen.16-9", "toshinden.widescreen.adaptive",
             "toshinden.widescreen.backdrop-preserve",
             "toshinden.boss-roster.enable", "toshinden.boss-roster.ui",
             "toshinden.desperation.any-health",
             "toshinden.framerate.display", "toshinden.framerate.60",
             "toshinden.framerate.90", "toshinden.framerate.120",
             "toshinden.framerate.144", "toshinden.framerate.165",
             "toshinden.framerate.240"}) {
        check(mod_register_activation_plugin(id, catalog_callback),
              std::string("catalog fixture must register ") + id);
    }
    check(mod_register_vblank_plugin("toshinden.desperation.any-health",
                                     catalog_callback),
          "catalog fixture must register the desperation vblank companion");
    // Addresses match the four function-entry constructors in src/mods.
    for (const auto& hook : {
             std::pair{"toshinden.widescreen.final-ot", 0x80197C70u},
             std::pair{"toshinden.widescreen.backdrop-prim", 0x80196874u},
             std::pair{"toshinden.boss-roster.select-helper", 0x8017A0D4u},
             std::pair{"toshinden.boss-roster.ui-final-ot", 0x80197C70u}}) {
        check(mod_register_function_entry_plugin(hook.first, hook.second,
                                                catalog_entry),
              std::string("catalog fixture must register ") + hook.first);
    }
}

static void check_unregistered_plugins_fail_closed(const fs::path& mods_root) {
    ModPackageManager manager(mods_root);
    std::string error;
    check(manager.scan(&error), error);
    check(manager.set_feature_enabled("toshinden.enhancement.widescreen",
                                      "widescreen", true, &error), error);
    const ModResolution resolution = manager.resolve("SCUS-94200", "", "");
    check(!resolution.ok && std::any_of(
              resolution.errors.begin(), resolution.errors.end(),
              [](const std::string& value) {
                  return value == "toshinden.enhancement.widescreen/widescreen: "
                                  "trusted plugin is unavailable: "
                                  "toshinden.widescreen.final-ot";
              }),
          "an absent trusted companion implementation must fail closed");
    check(resolution.plugins.empty(),
          "an unregistered catalog must not select trusted implementations");
}

static void check_package_resolves_without_exe_hash(const fs::path& mods_root,
                                                    const std::string& package_id,
                                                    const std::string& feature_id) {
    ModPackageManager manager(mods_root);
    std::string error;
    check(manager.scan(&error), error);
    check(manager.set_feature_enabled(package_id, feature_id, true, &error), error);

    ModResolution resolution = manager.resolve("SCUS-94200", "", "");
    for (const auto& resolution_error : resolution.errors)
        std::cerr << package_id << ": " << resolution_error << "\n";
    check(resolution.ok && resolution.errors.empty(),
          package_id + " must resolve with its trusted fixture registry");
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

    mod_clear_plugins_for_tests();
    check_unregistered_plugins_fail_closed(test_root);
    register_catalog_fixture_plugins();
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.enhancement.widescreen", "widescreen");
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.enhancement.frame-interpolation", "frame-interpolation");
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.gameplay.boss-roster", "boss-roster");
    check_package_resolves_without_exe_hash(
        test_root, "toshinden.gameplay.desperation-at-any-health",
        "desperation-at-any-health");
    mod_clear_plugins_for_tests();
    fs::remove_all(test_root, ec);
    return failures == 0 ? 0 : 1;
}
