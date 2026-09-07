#!/usr/bin/env bash
# package_appimage.sh -- build the Linux x86_64 AppImage release for Battle Arena Toshinden Recompiled.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
orig_args=("$@")

version=""
out_dir=""
skip_build=0
build_dir=${BUILD_DIR:-"$root/build-appimage"}
_cores=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
jobs=${BUILD_JOBS:-$(( _cores > 4 ? _cores - 2 : 2 ))}

while [ $# -gt 0 ]; do
    case "$1" in
        --version) version=${2:?}; shift 2;;
        --out) out_dir=${2:?}; shift 2;;
        --build-dir) build_dir=${2:?}; shift 2;;
        --jobs) jobs=${2:?}; shift 2;;
        --skip-build) skip_build=1; shift;;
        --nice) nice_level=${2:?}; shift 2;;
        -h|--help) sed -n '2,34p' "$0"; exit 0;;
        *) echo "unknown arg: $1" >&2; exit 2;;
    esac
done

nice_level=${nice_level:-10}
if [ "$nice_level" -gt 0 ] && [ "${RECOMP_APPIMAGE_RENICED:-0}" != "1" ] \
   && command -v nice >/dev/null 2>&1; then
    export RECOMP_APPIMAGE_RENICED=1
    exec nice -n "$nice_level" "$0" ${orig_args[@]+"${orig_args[@]}"}
fi

if [ -z "$version" ]; then
    [ -f "$root/VERSION" ] || { echo "missing VERSION; pass --version" >&2; exit 1; }
    version=$(tr -d ' \t\r\n' < "$root/VERSION")
fi
version=${version#v}
[ -n "$version" ] || { echo "empty version" >&2; exit 1; }

app_conf=$root/packaging/release/app.conf
[ -f "$app_conf" ] || { echo "missing $app_conf" >&2; exit 1; }
# shellcheck source=/dev/null
. "$app_conf"
ARTIFACT_NAME=${ARTIFACT_NAME:-$EXE_NAME}
GAME_TOML=${GAME_TOML:-game.toml}
runtime_target=${RUNTIME_TARGET:-psx-runtime}
for v in APP_NAME EXE_NAME PAYLOAD_DIR DESKTOP_ID ENV_PREFIX ICON_SOURCE FRAMEWORK_DIR; do
    eval "val=\${$v:-}"
    [ -n "$val" ] || { echo "$app_conf does not set $v" >&2; exit 1; }
done

to_unix_path() {
    case "$1" in
        [A-Za-z]:[/\\]*)
            if command -v wslpath >/dev/null 2>&1; then wslpath -u "$1"; else
                echo "cannot translate Windows path '$1' (wslpath missing)" >&2; exit 2
            fi;;
        *) printf '%s\n' "$1";;
    esac
}
[ -n "$out_dir" ] && out_dir=$(to_unix_path "$out_dir")
out_dir=${out_dir:-"$root/release-linux"}
mkdir -p -- "$out_dir"
out_dir=$(CDPATH= cd -- "$out_dir" && pwd)

is_wsl=0
if [ -r /proc/version ] && grep -qiE 'microsoft|wsl' /proc/version; then is_wsl=1; fi
if [ "$is_wsl" = "1" ]; then
    clean_path=""
    old_ifs=$IFS
    IFS=:
    for entry in $PATH; do
        case "$entry" in
            /mnt/*|/cygdrive/*|'') continue;;
        esac
        case ":$clean_path:" in
            *:"$entry":*) ;;
            *) clean_path=${clean_path:+$clean_path:}$entry;;
        esac
    done
    IFS=$old_ifs
    export PATH=${clean_path:-/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin}
    export CCACHE_DISABLE=1
fi
export CMAKE_C_COMPILER_LAUNCHER=""
export CMAKE_CXX_COMPILER_LAUNCHER=""
stage_base=$build_dir
case "$root" in
    /mnt/*)
        if [ "$is_wsl" = "1" ]; then
            stage_base=${TMPDIR:-/tmp}/$PAYLOAD_DIR-appimage.$$
            echo "WSL: repo is on DrvFs; staging AppDir at $stage_base"
        fi;;
esac
appdir=$stage_base/AppDir
tools_dir=${RECOMP_APPIMAGE_TOOLS:-"${XDG_CACHE_HOME:-$HOME/.cache}/recomp-appimage-tools"}
output=$out_dir/$ARTIFACT_NAME-v$version-linux-x86_64.AppImage

cleanup() {
    case "$stage_base" in
        /tmp/"$PAYLOAD_DIR"-appimage.*|"${TMPDIR:-/tmp}"/"$PAYLOAD_DIR"-appimage.*)
            rm -rf -- "$stage_base";;
    esac
}
trap cleanup EXIT

if [ -z "$(ls "$root"/generated/*_dispatch.c 2>/dev/null)" ]; then
    echo "Missing generated game sources (generated/*_dispatch.c)." >&2
    exit 1
fi

if [ -z "${SOURCE_DATE_EPOCH:-}" ]; then
    SOURCE_DATE_EPOCH=$(git -C "$root" log -1 --format=%ct 2>/dev/null || true)
    if [ -z "$SOURCE_DATE_EPOCH" ]; then
        SOURCE_DATE_EPOCH=$(stat -c %Y "$root/VERSION" 2>/dev/null || echo 0)
        echo "note: git date unavailable; SOURCE_DATE_EPOCH from VERSION mtime" >&2
    fi
fi
export SOURCE_DATE_EPOCH

echo "version=$version  SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH"

fw=$root/$FRAMEWORK_DIR
[ -d "$fw" ] || { echo "missing framework dir: $fw" >&2; exit 1; }
# shellcheck source=/dev/null
. "$fw/tools/release_overlay_stage.sh"
psx_release_stage_init "$fw"

bios_build=${PSXRECOMP_BIOS_BUILD:-recompiler/build-linux}
bios_rom_for() {
    case "$1" in
        OpenBIOS) printf '%s\n' "$fw/bios/openbios.bin";;
        *) printf '%s\n' "$fw/bios/$1.BIN";;
    esac
}
needed_stems=""
for stem in OpenBIOS SCPH1001; do
    [ -f "$fw/bios/$stem.toml" ] || continue
    [ -f "$(bios_rom_for "$stem")" ] || continue
    [ -f "$fw/generated/${stem}_dispatch.c" ] && continue
    needed_stems="$needed_stems $stem"
done
if [ -n "$needed_stems" ]; then
    if [ ! -x "$fw/$bios_build/psxrecomp-bios" ]; then
        gen=Ninja
        command -v ninja >/dev/null 2>&1 || gen="Unix Makefiles"
        cmake -S "$fw/recompiler" -B "$fw/$bios_build" -G "$gen" -DCMAKE_BUILD_TYPE=Release
        cmake --build "$fw/$bios_build" --target psxrecomp-bios -j "$jobs"
    fi
    for stem in $needed_stems; do
        echo "Generating recompiled BIOS backend: $stem"
        ( cd "$fw" && PSXRECOMP_BIOS_BUILD="$bios_build" tools/regen_bios.sh --config "bios/$stem.toml" )
    done
fi

if [ "$skip_build" = "0" ]; then
    generator=Ninja
    command -v ninja >/dev/null 2>&1 || generator="Unix Makefiles"
    extra=()
    if command -v glslc >/dev/null 2>&1; then
        extra+=("-DGLSLC_EXE=$(command -v glslc)")
    fi
    cmake -S "$root" -B "$build_dir" -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release \
        -DPSX_NETPLAY=ON \
        -DPSX_SETUP_WIZARD=ON \
        -DPSX_GAME_VERSION="$version" \
        -DPSX_SDL_BACKEND=SDL3 \
        -DCMAKE_DISABLE_FIND_PACKAGE_SDL3=TRUE \
        -DPSX_DEBUG_TOOLS=OFF \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        -DCCACHE_PROGRAM=CCACHE_PROGRAM-NOTFOUND \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,--build-id=none" \
        "${extra[@]}"
    cmake --build "$build_dir" --target "$runtime_target" -j "$jobs"
fi

elf=$build_dir/$EXE_NAME
[ -f "$elf" ] || elf=$build_dir/psx-runtime
[ -f "$elf" ] || { echo "no runtime ELF under $build_dir" >&2; exit 1; }
file -b "$elf" | grep -q ELF || { echo "$elf is not an ELF binary" >&2; exit 1; }

player_toml=$root/packaging/release/$GAME_TOML
[ -f "$player_toml" ] || player_toml=$root/game.toml
[ -f "$player_toml" ] || { echo "missing $GAME_TOML" >&2; exit 1; }

rm -rf -- "$appdir"
mkdir -p "$appdir/usr/bin" "$appdir/usr/share/$PAYLOAD_DIR"
payload=$appdir/usr/share/$PAYLOAD_DIR
install -m 0755 "$elf" "$appdir/usr/bin/$EXE_NAME"

sed -e "s|@VERSION@|v$version|g" \
    -e "s|@APP_NAME@|$APP_NAME|g" \
    -e "s|@EXE_NAME@|$EXE_NAME|g" \
    -e "s|@ARTIFACT_NAME@|$ARTIFACT_NAME|g" \
    -e "s|@PAYLOAD_DIR@|$PAYLOAD_DIR|g" \
    -e "s|@ENV_PREFIX@|$ENV_PREFIX|g" \
    -e "s|@GAME_TOML@|$GAME_TOML|g" \
    "$root/packaging/linux/AppRun" > "$appdir/AppRun"
chmod 0755 "$appdir/AppRun"
sed -e "s|^Name=.*|Name=$APP_NAME|" \
    -e "s|^Exec=.*|Exec=$EXE_NAME|" \
    -e "s|^Icon=.*|Icon=$DESKTOP_ID|" \
    "$root/packaging/linux/$DESKTOP_ID.desktop" > "$appdir/$DESKTOP_ID.desktop"

for tree in assets bios; do
    [ -d "$build_dir/$tree" ] || { echo "build did not stage $tree/" >&2; exit 1; }
    cp -a "$build_dir/$tree" "$payload/$tree"
done
mkdir -p "$payload/licenses"
if [ -f "$fw/LICENSE" ]; then
    cp "$fw/LICENSE" "$payload/licenses/psxrecomp-LICENSE"
fi
if [ -f "$fw/THIRD_PARTY_ATTRIBUTION.md" ]; then
    cp "$fw/THIRD_PARTY_ATTRIBUTION.md" "$payload/licenses/psxrecomp-THIRD_PARTY_ATTRIBUTION.md"
fi
if [ -f "$fw/lib/recomp-ui/LICENSE" ]; then
    cp "$fw/lib/recomp-ui/LICENSE" "$payload/licenses/recomp-ui-LICENSE"
fi
if [ -f "$fw/runtime/licenses/libchdr-NOTICES.txt" ]; then
    cp "$fw/runtime/licenses/libchdr-NOTICES.txt" "$payload/licenses/"
fi
psx_add_mod_catalog --build-path "$build_dir" --stage "$payload" --runtime-target "$runtime_target"

cp "$player_toml" "$payload/$GAME_TOML"
[ -f "$root/game_options.toml" ] && cp "$root/game_options.toml" "$payload/game_options.toml"
cp "$root/packaging/release/input.ini" "$payload/input.ini"
cp "$root/packaging/release/START_HERE.txt" "$payload/START_HERE.txt"
cp "$root/README.md" "$payload/README.md"
cp "$root/VERSION" "$payload/VERSION"
[ -f "$root/RELEASE_NOTES.md" ] && cp "$root/RELEASE_NOTES.md" "$payload/RELEASE_NOTES.md"
[ -f "$root/THIRD_PARTY_ATTRIBUTION.md" ] && cp "$root/THIRD_PARTY_ATTRIBUTION.md" "$payload/THIRD_PARTY_ATTRIBUTION.md"
[ -f "$root/LICENSE" ] && cp "$root/LICENSE" "$payload/LICENSE"

ln -s "../share/$PAYLOAD_DIR/assets" "$appdir/usr/bin/assets"

if command -v magick >/dev/null 2>&1; then image_tool=magick
elif command -v convert >/dev/null 2>&1; then image_tool=convert
else echo "ImageMagick is required for the AppImage icon." >&2; exit 1; fi
"$image_tool" "$root/$ICON_SOURCE" \
    -resize 240x240 -background transparent -gravity center -extent 256x256 \
    "$appdir/$DESKTOP_ID.png"
ln -s "$DESKTOP_ID.png" "$appdir/.DirIcon"

linuxdeploy_url=https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
linuxdeploy_sha=36a2d7e274d12e1050d0e9ecfe11d339ed54720b2bec464c286d53f8b07f5c62
appimagetool_url=https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
appimagetool_sha=a6d71e2b6cd66f8e8d16c37ad164658985e0cf5fcaa950c90a482890cb9d13e0
mkdir -p "$tools_dir"
fetch_tool() {
    url=$1; sha=$2; dest=$3
    if [ ! -f "$dest" ] || [ "$(sha256sum "$dest" | awk '{print $1}')" != "$sha" ]; then
        curl -fL --retry 3 "$url" -o "$dest.tmp"
        printf '%s  %s\n' "$sha" "$dest.tmp" | sha256sum -c -
        mv "$dest.tmp" "$dest"
    fi
    chmod 0755 "$dest"
}
linuxdeploy=$tools_dir/linuxdeploy-x86_64.AppImage
appimagetool=$tools_dir/appimagetool-x86_64.AppImage
fetch_tool "$linuxdeploy_url" "$linuxdeploy_sha" "$linuxdeploy"
fetch_tool "$appimagetool_url" "$appimagetool_sha" "$appimagetool"

export NO_STRIP=1
APPIMAGE_EXTRACT_AND_RUN=1 "$linuxdeploy" \
    --appdir "$appdir" \
    --executable "$appdir/usr/bin/$EXE_NAME" \
    --desktop-file "$appdir/$DESKTOP_ID.desktop" \
    --icon-file "$appdir/$DESKTOP_ID.png"

find "$appdir" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} + 2>/dev/null || true
rm -f -- "$output"
APPIMAGE_EXTRACT_AND_RUN=1 ARCH=x86_64 "$appimagetool" "$appdir" "$output"
chmod 0755 "$output"
sha256sum "$output"
echo "AppImage: $output"