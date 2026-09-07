#!/usr/bin/env bash
# Verify AppImage data seeding without launching the game.
set -euo pipefail

appimage=${1:-}
[ -n "$appimage" ] || { echo "usage: $0 <AppImage>" >&2; exit 2; }
[ -x "$appimage" ] || { echo "not executable: $appimage" >&2; exit 1; }

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
expected_version=$(tr -d ' \t\r\n' < "$root/VERSION")
expected_version=v${expected_version#v}
# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"
GAME_TOML=${GAME_TOML:-game.toml}

work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT

data_dir=$(env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run)
[ -n "$data_dir" ] || { echo "AppRun printed no data dir" >&2; exit 1; }

fail=0
check_file() { [ -f "$data_dir/$1" ] || { echo "MISSING file: $1" >&2; fail=1; }; }
check_dir() { [ -d "$data_dir/$1" ] || { echo "MISSING dir: $1" >&2; fail=1; }; }

for d in saves cache mods assets bios; do check_dir "$d"; done
for f in "$GAME_TOML" input.ini START_HERE.txt README.md bios/openbios.bin bios/OpenBIOS.LICENSE .appimage-layout-version; do
    check_file "$f"
done

got_version=$(tr -d ' \t\r\n' < "$data_dir/.appimage-layout-version")
if [ "$got_version" != "$expected_version" ]; then
    echo "version marker mismatch: AppImage says '$got_version', VERSION says '$expected_version'" >&2
    fail=1
fi

manifests=$(find "$data_dir/mods" -name manifest.toml | wc -l)
if [ "$manifests" -lt 1 ]; then
    echo "seeded mod catalog has no manifests" >&2
    fail=1
fi

stray=$(find "$data_dir" \( -iname 'SCPH*.BIN' -o -iname '*.cue' -o -iname '*.iso' -o -iname '*.mcd' \) -print 2>/dev/null || true)
if [ -n "$stray" ]; then
    echo "payload contains files that must never ship:" >&2
    printf '  %s\n' $stray >&2
    fail=1
fi

env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run >/dev/null

echo "; user edit" >> "$data_dir/input.ini"
before=$(sha256sum "$data_dir/input.ini" | awk '{print $1}')
env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run >/dev/null
after=$(sha256sum "$data_dir/input.ini" | awk '{print $1}')
if [ "$before" != "$after" ]; then
    echo "reseed clobbered user-owned input.ini" >&2
    fail=1
fi

if [ "$fail" -ne 0 ]; then
    echo "AppImage layout test FAILED" >&2
    exit 1
fi
echo "AppImage layout test passed ($expected_version)"