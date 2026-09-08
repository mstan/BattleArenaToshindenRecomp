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
