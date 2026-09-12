# Validated baseline

The September 11, 2026 [original-disc overlay assessment](../docs/OVERLAY_ASSESSMENT.md)
found no executable overlays in the inspected supported USA build. It records
original hashes, raw-sector loader and consumer evidence, and reproducible byte
checks; no additional overlay AOT shards or binary release were needed.

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

## Initial release packaging, September 7, 2026

- Removed the Extra Roster navigation hint and its bottom background panel.
  Rebuilt and visually checked the Gaia/Sho selector, then refreshed its README
  screenshot. Cleanup committed separately as `18ff91b`.
- Windows release uses the validated game code with debug tools disabled,
  static runtime dependencies, SDL3, and lobby version `0.1.0`.
- A fresh extraction of the Windows ZIP booted with bundled OpenBIOS into
  Eiji versus Ellis combat and accepted native keyboard input. No retail BIOS,
  saved configuration, or extracted executable was copied into the test package.
- Windows packaging checked system-only DLL imports, all eight bundled mod
  packages, clean player defaults, and exclusion of disc data, retail BIOS,
  generated source files, saves, and developer mod selections.
- Linux AppImage built with SDL3 and debug tools disabled under Ubuntu 24.04
  in WSL. Layout checks passed, including fresh data seeding, version `0.1.0`,
  the bundled catalog, and preservation of an existing input mapping.
- The actual AppImage ran a 20-second startup smoke with bundled OpenBIOS,
  the user's disc, Mesa offscreen OpenGL, and dummy audio. Runtime initialization
  passed; this was not a complete Linux gameplay or audio validation.
- Native release scripts share root `VERSION` and use the framework's mod
  staging helpers. The generic setup-host workflow remains manual so a native
  release tag does not publish competing setup-only downloads.


## 0.1.1 online Extra Roster and selector cleanup, September 7, 2026

- Forward-ported the shared online mod-plan backend into the title's pinned
  framework. The host publishes required package versions, enabled features,
  options, and a matching plan fingerprint. Guests advertise installed packages
  even when their offline features are disabled. Missing fingerprints, unknown
  features/options, and resource-backed plans fail before game launch.
- Two isolated Windows clients joined a password-protected room through
  `ws://netplay.retcomm.net:8765`. Both launched with `force_input_relay=1`,
  slots 0/1, and public relay `netplay.retcomm.net:8777`. Match traffic used
  the public server; both test clients themselves ran on one computer.
- The host enabled Extra Roster; the guest started with no saved mod selection.
  The guest's View Mods showed the host-required package as installed. Guest
  offline state remained absent after the session, confirming it was not saved.
- Independent player input selected Gaia (8) for P1 and Sho (9) for P2 on both
  clients. Left/Right retained the extra selections; confirm plus direction
  entered Gaia versus Sho. Both clients rendered the revised selector and combat.
- Combat inputs reached the corresponding player on both peers. Of 18 timed
  pad samples, 17 showed the current held input and one showed the preceding
  input on both peers during correction. This is functional input coverage,
  not a guarantee of fixed network input latency.
- The completed match run produced 416 shared sampled core-state digests
  through simulation tick 13280; all matched. A transient prediction divergence
  occurred and the run continued.
- Removed the EXTRA heading/background and small GAIA/SHO icon captions;
  preserved large portrait names and player badges. Refreshed the README image
  from the actual online selector. No navigation hint remains.
- Removed host-only remembered roster positions and saved highlight colors.
  Selector and highlight regression harnesses passed including RAM restoration
  after a divergent host timeline. Returning to the regular row now consistently
  selects Eiji for P1 and Kayin for P2. This supersedes the earlier remembered
  return-slot save-state limitation documented above.
- Fixed the guest lobby summary to read the host's plan instead of the guest's
  offline selection. The launcher source built successfully on Windows/Linux.

An earlier attempt to skip the opening movie exposed a shared rollback-engine
failure: abort realignment removed a ring snapshot while a pinned copy survived;
media-keyframe retry ignored that copy and eventually returned to the lobby.
The included focused fix makes payload lookup, CRC probing, and sealing use the
matching pinned baseline. Its regression passed for evicted ring snapshots,
matching/wrong ticks, and preserving the pinned payload. The original live
movie-skip failure was not re-exercised after this fix. Adverse-network recovery
and independent remote networks have not been exhaustively tested.

Release acceptance is the working online lobby/mod UI and functional Gaia/Sho
selection/combat, as requested. Automatic mod transfer is not implemented in
this focused backend port; its download UI is disabled. Resource-backed online
mods are explicitly unsupported. LAN / Direct IP retains vanilla sessions.


## 0.1.1 package checks

- Windows and Linux Release builds passed with debug tools disabled and game
  version 0.1.1. Both contain the four title mods and four framework packages.
- Windows ZIP checks passed for system-only DLL imports, clean defaults, and
  exclusion of discs, retail BIOS files, saves, and developer mod selections.
  A fresh extraction booted with bundled OpenBIOS to the character selector.
- Linux AppImage layout and data-seeding tests passed for version 0.1.1,
  including preservation of an existing input mapping. The actual AppImage ran
  for 20 seconds using bundled OpenBIOS and Mesa offscreen OpenGL under WSL;
  BIOS, disc, and GPU initialization passed. This was a startup smoke, not a
  full Linux playthrough. The smoke script's initial case-sensitive BIOS label
  check was corrected to accept the runtime's uppercase OPENBIOS label.


## Post-0.1.1 widescreen score fix, September 7, 2026

- Reproduced split scores at 21:9 with eight-digit values for both players.
  The generic horizontal anchor heuristic assigned different anchors to digits
  in the same score. Both native eight-SPRT score pools (`bank+0x1E0` and
  `bank+0x280`, stride `0x14`) now remain grouped relative to the timer.
- Rebuilt the Windows game and inspected live captures at 4:3, 16:9, 21:9,
  32:9, and after resizing back to 21:9. Temporarily injected scores `12345678`
  and `87654321` appeared intact at every size. All eight glyph cells remained
  visible; score bounds relative to screen center varied by at most three
  pixels from the 4:3 capture due to raster sampling. Restored the original
  zero-score state and checked the pause screen at 21:9.
- Health bars still extend to the edges. Names remain edge-anchored; the timer
  and pause text retain their proportions. The user also confirmed the scores
  look fixed in the running build.
- Standalone callback regression passed with native SPRT layouts for every
  score digit in both HUD banks, neighboring HUD roles, disabled-mod behavior,
  and 4:3 passthrough. Run from the title root:
  `clang -std=c99 -Wall -Wextra -Werror -Itests/stubs -I. tests/test_widescreen_scores.c -o validation/test_widescreen_scores.exe`,
  then run `validation/test_widescreen_scores.exe`.
- Restored the README Discord badge using the existing Super Mario World
  repository asset and removed internal process details from the player README.

These checks cover the source update and local Windows build. The published
0.1.1 packages predate this score fix.


## Post-0.1.1 digital controls and alternate portraits, September 7, 2026

- Locked Toshinden to its native digital controller protocol with the existing
  `game.toml` controller lock. The launcher hides analog-mode selection while
  retaining physical controller selection. An isolated launcher with a connected
  DualSense and stale `p1_mode="analog"` showed digital pad art and no mode
  selector. Runtime pad status reported both ports digital; Multitap stays off.
- Fixed Extra Roster overwriting native alternate portrait textures. The mod
  now appends GPU upload/draw/restore commands after native selector drawing,
  restoring both complete 128x128 regions and the texture-window/mask state.
  Native transfer commands cause the mod to skip that frame, and packet-buffer
  overflow fails closed. Disc assets and the shared framework are unchanged.
- Compared all eight regular characters in both normal and alternate states:
  all 16 portrait interiors matched clean mod-disabled captures pixel for pixel.
  Verified both complete borrowed VRAM regions equal the original texture data.
  The final binary also passed a native pause-menu Reset, return to character
  select, and alternate portrait check without a state load on that return.
- Confirmed Gaia and Sho portraits/cards render and their explicit alternate
  choices survive confirmation against different CPU opponents. Both entered
  combat. Captures show blue-armored Gaia and Sho with reddish hair and orange
  clothing in their native alternate costumes. The mod still has one portrait
  per extra character; matching alternate artwork is a separate follow-up.
- Windows Release build passed. Focused tests use production mod code:
  `tests/test_portrait_uploads.c` covers GPU stream order, masked native pixels,
  exact restoration, environment state, transfer rejection, and buffer failure;
  `tests/test_extra_roster.c` covers both players' explicit costume choices and
  automatic mirror colors. The existing widescreen score regression and shared
  launcher pad-mode resolution guard also passed.

Compile either title regression with
`clang -std=c99 -Wall -Wextra -Werror -Itests/stubs -I. tests/<test>.c -o validation/<test>.exe`
and run the result. These fixes are validated in the local Windows build;
0.1.1 release assets predate them. No new online or Linux runtime session was
run for this follow-up.


## Post-0.1.1 Gaia and Sho alternate artwork, September 7, 2026

- Added blue-armored Gaia and auburn-haired, orange-clad Sho alternate portraits,
  based on native USA costume captures. Both use distinct opposite-facing poses.
  Select changes the owning player's large portrait; small roster cards retain
  default art. The Extra Roster package is now 1.0.1 so online mod matching can
  distinguish this asset and command-stream revision.
- Inspected live Gaia and Sho mirror selections with different variants, mixed
  regular/extra selections, and Select toggling. Rechecked Eiji, Kayin, Sofia,
  and Rungo alternate portrait interiors: all four matched clean native captures
  pixel for pixel. The production-code packet regression passed per-player
  variants, ordered uploads, exact native texture restoration, mask/window
  restoration, and buffer limits. Live VRAM peeks can occur during a transient
  upload; the packet test checks restoration after the complete stream.
- Fixed the garbled save-state load: the framework now saves mod guest-memory
  and GPU command buffers with the machine state. The title reserves its two
  buffers during mod activation. New selector states reloaded successfully;
  an old state without those buffers was rejected safely. The focused framework
  regression covers payload/cursor restoration and rejection before mutation.
  This is a focused save/load check, not an exhaustive save-state compatibility
  audit. Older states without mod memory cannot load with these allocations active.
- Windows Release build and focused portrait/framework regressions passed.
  This follow-up did not repeat Linux runtime or online sessions. Published
  0.1.1 release assets predate these changes.


## 0.1.2 release checks, September 7, 2026

This release includes the score, digital-control, native portrait preservation,
Gaia/Sho alternate artwork, and mod-buffer save-state fixes described above.
The user approved the final in-game appearance before packaging.

- Windows Release package built with game version 0.1.2 and debug tools off.
  Checked the actual ZIP: eight bundled packages, Extra Roster 1.0.1, digital
  mode locked, and Multitap disabled. Only system DLL imports are required;
  package checks exclude disc files, retail BIOS, saves, and local settings.
- A fresh Windows extraction using bundled OpenBIOS reached character selection
  with clean native portraits. The release-specific game.toml now includes the
  same controller lock as development builds.
- Linux x86_64 AppImage built and passed layout/data-seeding tests, including
  preservation of existing input mappings. Its payload contains version 0.1.2,
  eight mods, Extra Roster 1.0.1, and locked digital controls without Multitap.
  A 20-second WSL offscreen smoke initialized bundled OpenBIOS, the USA disc,
  and Mesa OpenGL. This Linux package uses OpenGL; Vulkan headers were absent
  from its build environment. Both builds retained the existing BIOS-source
  fingerprint warning and used the already-tested generated BIOS backend.
- A bounded GPT-5.5 review found no omissions in source inclusion, mod staging,
  controller configuration, version stamping, or the pinned save-state fix.

The focused portrait and save-state checks above remain the validation scope;
this release does not claim an additional exhaustive online or Linux playthrough.
