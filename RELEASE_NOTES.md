# 0.1.1 - Online Extra Roster

Gaia and Sho can now be selected in online matches. The host chooses Extra
Roster from the lobby's Mods page; guests see the required mod in View Mods
and receive the host's selection automatically when the match launches.

- Added online mod-plan exchange, installed-version checks, and matching
  plan fingerprints. Guest offline mod settings remain saved separately.
- Verified the online lobby flow, Gaia/Sho selection, and combat through the
  public relay with independently controlled players.
- Removed the EXTRA heading and the small GAIA/SHO icon captions. Character
  names beneath the large portraits remain.
- Made extra-row navigation and selection highlights deterministic when
  game state is restored. Returning to the regular row selects Eiji for P1
  and Kayin for P2.
- Corrected the guest lobby summary to show the host's mod selection.

Windows x64 ZIP and Linux x86_64 AppImage are included. The game is believed
to be stable based on gameplay testing and user validation despite its early
version. Online testing is functional, not exhaustive; see the validation
notes for scope and known limits. Automatic mod downloads and resource-backed
online mods are not supported. LAN / Direct IP sessions run without mods.

This release replaces 0.1.0, which has been hidden as a draft.

Provide your own USA 14-track BIN/CUE disc image. No disc data or retail BIOS
is included; MIT-licensed OpenBIOS is bundled. Linux requires glibc 2.38 or
newer. See README.md for setup instructions.
