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

These checks cover the display milestone. Boss selection and desperation-move
mods are separate follow-up work. Captures, measurement scripts, RAM dumps,
and process logs remain local and ignored.

### Attract-mode lettering follow-up

The user subsequently found that `DEMONSTRATION` split into three widely
spaced groups. Its 13 large glyph quads occupy native y=32..48, within the
player HUD's vertical band. The classifier now identifies the dedicated
320-quad overlay font pool initialized at each UI bank's offset 0x1428 and
centers those glyphs instead of assigning each letter a player-side anchor. Presented
captures at 4:3, 21:9 and 32:9 confirmed one centered word of constant width;
the normal fight HUD and pause panel were rechecked at 21:9. Health gauges
still fill the available width; player labels and win markers remain at the edges.
