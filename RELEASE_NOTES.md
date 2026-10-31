# 0.1.4 - Shared Optimized Release Pipeline

- Adopted the reviewed shared engine and current launcher, with fresh
  title-specific selective PGO through the maintained release builder.
- Fresh profiles enable Internal Resolution at 4x, with 1x through 32x
  choices subject to GPU texture and memory admission. Saved mod choices
  remain supported. CD Speed and Fast Loading packages are excluded.
- Retained the current declared widescreen and hidden-roster hooks, Gaia/Sho
  portraits, digital controller lock, native memory cards and negotiated
  two-player netplay configuration. The player config now preserves the
  source's content-negotiation gate.
- Updated the existing mod-target fixture's dependencies for the current
  resource reader, without changing its original hook/target assertions.

This release supplies a Windows x64 public ZIP with OpenBIOS. Original game
media, retail BIOS, personal saves/settings and private compiler profiles are
not bundled. Source retains the shared Linux AppImage build route; the earlier
v0.1.3 Linux artifact remains available separately.

Fresh profiling and package checks do not establish a new per-title speedup,
broader gameplay coverage or a new netplay qualification. Earlier observations
and their limits remain in the validation notes. No old snapshot compatibility
or migration is required; native memory-card progress remains separate.

# 0.1.3 - Clean Install Mod Target Fix

- Fixed bundled Toshinden mod packages being rejected on clean player installs
  with "package does not target this game/image", including Widescreen,
  Frame Interpolation, Extra Roster, and Desperation at Any Health.

Includes Windows x64 ZIP and Linux x86_64 AppImage builds. This is a targeted
hotfix over 0.1.2.

Provide your own USA 14-track BIN/CUE disc image. MIT-licensed OpenBIOS is
bundled; no disc data or retail BIOS is included. Linux requires glibc 2.38
or newer. See README.md for setup instructions.

# 0.1.2 - Alternate Portraits and Visual Fixes

- Added distinct alternate portraits for Gaia and Sho, with opposite-facing
  poses and colors matching their original alternate costumes. Press Select
  (Right Shift on keyboard) to switch. Both players choose independently.
- Fixed Extra Roster corrupting the original fighters' alternate portraits.
- Fixed split player scores in adaptive ultrawide views.
- Locked controller mode to the game's supported digital D-pad protocol while
  retaining physical controller selection.
- Fixed save-state restoration of mod GPU command buffers. Older states that
  lack the required mod buffers are rejected safely.
- Refreshed the README and restored the community Discord badge.

Includes Windows x64 ZIP and Linux x86_64 AppImage builds. This early release
is believed to be stable based on gameplay testing and user validation.

Two-player netplay and online Extra Roster remain supported. Both players
should update to 0.1.2; Extra Roster now identifies itself as package 1.0.1.
LAN / Direct IP sessions run without mods. See validation/README.md for scope.

Provide your own USA 14-track BIN/CUE disc image. MIT-licensed OpenBIOS is
bundled; no disc data or retail BIOS is included. Linux requires glibc 2.38
or newer. See README.md for setup instructions.
