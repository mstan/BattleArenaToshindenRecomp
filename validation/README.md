# Validated baseline

Battle Arena Toshinden (USA), SCUS-94200, was stood up with the framework's
New Project Layout and `psxrecomp_cli.py` Generate / rebuild workflow.

- Full 14-track disc verified by the probe; 424 initial seeds produced 949
  dispatch entries across 22 generated C shards.
- Windows Release build completed with the installed RetComM toolchain.
- Runtime reached the introduction, player selection, and Eiji versus Rungo
  gameplay. Presented OpenGL captures showed fighters, stage, HUD, portraits,
  and text rendering coherently. Audio telemetry showed active SPU, CD audio,
  and host output.
- The user confirmed the game was fully good and approved this baseline for
  enhancements on 2026-09-06. No title-specific enhancements were added.

Framework pin: `8d56cf659fd84367c3f778e17851674ac7896371`.
Launcher pin: `bba6266d740738d276a4769aadecff517d193f12`; this provides the API
required by the framework. The setup script's default launcher master pin
`773155a` did not compile with this framework revision.

Local screenshots and runtime logs remain in this directory, ignored by Git.
Disc images, BIOS images, generated game C, and build output are also ignored.

To reproduce from the title root with Python and the RetComM toolchain:

```text
python psxrecomp/psxrecomp_cli.py generate --config game.toml --project-root . --disc "iso/Battle Arena Toshinden (USA).cue" --bios "<your SCPH1001.BIN>"
python psxrecomp/psxrecomp_cli.py rebuild --config game.toml --project-root . --build-dir build-release --no-pgo --cmake-extra=-DPSX_DEBUG_TOOLS=ON
```

The playable executable is `build-release/BattleArenaToshinden_Recompiled.exe`.
Retail BIOS is optional for generation; the validation run used SCPH1001.

## Display enhancements validated September 7, 2026

- Windows Release build passed with the title's optional widescreen and frame
  blending packages staged in the launcher's Mods catalog. Both default off.
- Adaptive presentation was captured at 960x720, 1280x720, 1680x720, and
  2560x720, plus a square window and a return to 960x720. The game switched
  between original 4:3 and wider projection as the window resized.
- In the paused Eiji/Sofia battle, image measurements confirmed that the pause
  panel retained its width and stayed centered throughout the resize sequence.
  Health gauges grew between fixed outside margins and the centered timer;
  labels, win markers, and pause text retained their authored proportions.
- Presented captures showed wider stage geometry and a continuous, correctly
  scaled panorama. The game uses the framework's projection-squash path before
  CPU screen bounds; the automatic cull scan found no safe additional hooks.
  This validates the observed scenes, not every stage/camera combination.
- Options opened over a paused battle retained their centered layout at 21:9.
  The introduction, title menu, and character selection remained native 4:3.
- Disabling the widescreen mod and loading the same battle restored the
  original pillarboxed presentation at 21:9. Local PGXP and frame blending
  selections were preserved, and widescreen was re-enabled after this check.
- Frame blending ran alongside adaptive widescreen: runtime telemetry reported
  enabled interpolation, two history frames, and measured/target presentation
  at 165 Hz. The plugin changes presentation settings only.
- The standalone runtime packet-guard regression test passed. Explicit UI tags
  fingerprint their command packets to reject stale tags on reused RAM.

Captures, measurement scripts, RAM dumps, and process logs remain local and
ignored. Gameplay validation is recorded below.

### Attract-mode lettering follow-up

The user subsequently found that `DEMONSTRATION` split into three widely
spaced groups. Its 13 large glyph quads occupy native y=32..48, within the
player HUD's vertical band. The classifier now identifies the dedicated
320-quad overlay font pool initialized at each UI bank's offset 0x1428 and
centers those glyphs instead of assigning each letter a player-side anchor. Presented
captures at 4:3, 21:9 and 32:9 confirmed one centered word of constant width;
the normal fight HUD and pause panel were rechecked at 21:9. Health gauges
still fill the available width; player labels and win markers remain at the edges.


## Gameplay mods validated September 7, 2026

- The Release build stages eight packages: four title packages and four shared
  framework packages. Boss Roster and Desperation at Any Health both default off
  and target the USA executable hash.
- Boss selection uses the native character IDs (Gaia 8, Sho 9), confirmation,
  model loading, and combat paths. Generated portrait artwork occupies unused
  selection-screen VRAM; no original disc assets are redistributed.
- P1 selected each boss and entered combat. In VS HUMAN, actual second-controller
  keyboard input independently selected P2's boss. Gaia versus Sho entered a
  match; a Gaia mirror match retained distinct native red/blue costumes.
- Both players returned from the boss column to their previous regular fighter
  with the original portraits and cursors restored. Saving and loading the boss
  selection restored both portraits and selection indicators. Loading screens
  retained the boss portraits and native VS graphic while the added selector
  disappeared with the rest of the selection menu.
- VS COMPUTER also exposes the boss column when selecting the CPU opponent.
  The native selection handler and linked thumbnail packets guard the added UI.
- A fresh launch with Boss Roster disabled showed the original eight-character
  menu; Up did not select either boss for either player.
- Eiji's normal desperation command was exercised for both player slots at full
  health (0 accumulated damage out of 896 maximum). With the mod enabled, the
  command matcher progressed through all three direction tokens and started
  desperation action 0xB5. With it disabled, the same full-health command was
  rejected. At low health (720 accumulated damage), the disabled-mod control
  still started action 0xB5. This checks both command acceptance and execution;
  every character's distinct move command has not been individually exercised.
- A save state made with the instruction already patched initially exposed a
  dispatch-invalidation bug. The plugin now re-arms the four-byte executable
  range each VBlank. Both players' full-health commands passed after reload.
- The standalone packet-guard regression test and assertions over seven live
  desperation traces passed. GPT-5.5 adversarial source review found no remaining
  concrete blocker after the menu-state, cursor, and save/load fixes.

The validation used temporary, separate P2 keyboard bindings. Original local
controller preferences were restored afterward. The mods use the normal launcher
selection and restart workflow.


## Extra Roster refinement, September 7, 2026

- Public heading and package name are now EXTRA / Extra Roster. Stable internal
  package and plugin IDs remain unchanged so existing mod selections carry over.
- Vertical navigation matches the layout: Gaia above Sho above the eight-character
  row. Up cycles roster -> Sho -> Gaia -> roster; Down reverses that order.
  Left/Right is native roster movement on the bottom row and a no-op on extras.
- A live VS HUMAN run passed 34 assertions using separate native P1/P2 keyboard
  inputs. Both directions, row wrapping, returning to the current session's last
  regular selection, and Left/Right no-op behavior were covered. Confirm combined
  with Right kept each extra selected and entered Gaia versus Sho successfully.
- Adversarial review caught the confirm/direction leak; the mod now strips every
  direction bit on extra-row confirm before passing confirmation to native code.
- Replacement portraits were generated from character-wiki references and local
  original-PS1 gameplay captures. Gaia uses his enclosed first-game armor; Sho
  uses muted brown hair and a purple cowl. The same new art was visually checked
  in the small icons, large portraits, and versus loading screen.
- Release build and package staging passed. Reference captures and the live
  input trace remain ignored; artwork source notes and prompts accompany the PNGs.

Existing save-state limitation: the selected Gaia/Sho ID survives loading, but
its remembered regular-roster return slot is host-session state, not serialized.
Loading a state saved on an extra can therefore return to the current session's
last regular selection (or its default after restart). This revision does not
add framework save-state serialization or reserve bytes in retail game structs.

## Netplay and publication validation, September 7, 2026

- Enabled the actual recomp-net and lobby targets in the title build, with
  the shared online lobby URL and a two-player maximum. Release build passed.
- Two independent runtime instances used LAN UDP over loopback, rollback mode,
  separate writable save directories, and the same USA disc / SCPH1001 BIOS.
- P2 Start joined the selection screen; separate peer inputs selected fighters
  and entered Kayin versus Kayin. Movement and attack inputs reached the correct
  standard controller port on both peers in all 20 sampled routing checks.
- Compared 416 core-state digest samples through simulation tick 13280: all
  matched. Neither peer's diagnostic samples reported a desync. This covers
  roughly 3.7 minutes of simulation, including boot, menus, and combat.
- Launcher visibly exposes NETPLAY and connects to the online lobby browser.
  Two-player sessions use standard ports;
  no Multitap is armed. Disabled the optional Multitap analog extension in the
  title configuration and both Multitap settings in the local installation.
- GPT-5.5 adversarial review confirmed local-input-to-session-slot routing and
  the two-player port limit. Internet / NAT traversal and adverse-network
  rollback correction were not exercised by this loopback test.
- The framework deliberately clears all mods for vanilla netplay sessions.
  Offline enhancement validation above remains separate from this test.
- README images are actual presentation captures: native 4:3 title screen,
  revised EXTRA selection, and 21:9 Gaia versus Sho gameplay. No generated
  mockups are used for the screenshots.
