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
