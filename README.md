# Battle Arena Toshinden Recompiled

> This recompilation is a byproduct of developing
> [psxrecomp](https://github.com/mstan/psxrecomp). These are in-development
> previews, with more testing and polish still to come. Contributions,
> testing, issues, and PRs to the game or framework are welcome.
> Read more: [Recomp + AI: 5 Months Later](https://1379.tech/recomp-ai-5-months-later/).

Static recompilation of **Battle Arena Toshinden (USA)**, PlayStation serial
**SCUS-94200**, to native code using [psxrecomp](https://github.com/mstan/psxrecomp)
and [recomp-ui](https://github.com/mstan/recomp-ui).

## Status

The game boots through its introduction and character selection into combat.
The initial playable baseline was validated and approved for enhancements on
September 6, 2026. The launcher includes disc setup, settings, and a Mods page.
See [validation notes](validation/README.md) for the tested build and scope.

## Playing

Select your legally obtained **USA BIN/CUE disc image** in the launcher, use
Generate & Build when prompted, and launch the game. Keep all 14 tracks beside
the CUE file so the game can play its CD audio.

The framework supplies MIT-licensed OpenBIOS. A supported retail BIOS dump is
optional; validation used SCPH1001. Disc images and retail BIOS files are not
included in this repository.

Enable enhancements on the launcher's **Mods** page. The title's widescreen,
frame blending, and optional gameplay packages are disabled by default. Saves and
mod selections are local to the installation.

## Layout

- `iso/` - local disc image and audio tracks, ignored by Git.
- `disc/` - extracted `SCUS_942.00` boot executable and disc files, ignored by Git.
- `seeds/` - function entry points used by the recompiler.
- `generated/` - generated native C, rebuilt locally and ignored by Git.
- `src/mods/` - title-specific native mod plugins.
- `mods/preloaded/` - bundled mod manifests and documentation.
- `psxrecomp/` and `recomp-ui/` - pinned framework and launcher submodules.
- `game.toml` - disc identity, compilation, and runtime configuration.

## Build

Initialize the pinned submodules, then use the framework's setup commands
from this directory with Python 3 and the supported RetComM toolchain:

```sh
git submodule update --init --recursive
python psxrecomp/psxrecomp_cli.py generate --config game.toml --project-root . --disc "iso/Battle Arena Toshinden (USA).cue"
python psxrecomp/psxrecomp_cli.py rebuild --config game.toml --project-root . --build-dir build-release --no-pgo
```

The Windows executable is `build-release/BattleArenaToshinden_Recompiled.exe`.
The launcher also exposes the same Generate & Build workflow. See the
[framework setup guide](psxrecomp/docs/GAME_PROJECT_SETUP.md) for prerequisites
and other platforms. SDL3 is the default host backend; SDL2 is an explicit
compatibility build option.

Submodule gitlinks are the authoritative framework versions. Generated code,
disc data, local settings, saves, and build output do not belong in commits.

## Built-in mods

**Battle Arena Toshinden Widescreen** offers fixed 16:9 and Adaptive. Adaptive
follows the current window or fullscreen aspect from 4:3 through 32:9, the
framework's current limit. Wider gameplay views reveal more of the stage;
health gauges extend to the perimeter while labels, icons, timer, and pause UI
retain their authored proportions. True 2D screens retain their 4:3 presentation.

**Battle Arena Toshinden Frame Blending** presents completed game frames at
the display's measured refresh rate or a selected 60, 90, 120, 144, 165, or
240 Hz target. Motion-adaptive blending reduces trails on large image changes.
Game simulation, inputs, timers, and audio retain their original cadence.
This is temporal image blending, not motion-vector frame generation.

**Battle Arena Toshinden Boss Roster** adds Gaia and Sho as optional selectable
fighters. The normal eight-character roster stays in place; Up/Down switches a
player between the regular roster and a central boss column, Left/Right chooses
Gaia or Sho inside that column, and the normal confirm button starts the match
through the game's selection path. The package uses new generated portrait art;
source details are documented in `assets/mods/boss-roster/SOURCE.md`.

**Battle Arena Toshinden Desperation at Any Health** lets players perform
each character's normal desperation command at any health. It does not add a
one-button shortcut and does not change health, damage, KO handling, max HP, or
HUD gauges.

The shared framework also supplies standard enhancement packages, including
PGXP. Each package's description and options are available on the Mods page.

## License

No game disc data or retail PlayStation BIOS is redistributed here. OpenBIOS
is provided under its MIT license; see the framework's OpenBIOS license notice.
Launcher box art attribution is recorded in
[BOXART_SOURCE.txt](launcher_assets/img/BOXART_SOURCE.txt).

<!-- retcomm-readme-raid -->
---

<p align="center">
  <sub><b>R.A.I.D. - Retro AI Development</b> / a Discord for AI-assisted retro reverse-engineering, decomp &amp; recomp</sub>
</p>

<p align="center">
  <a href="https://discord.gg/Ad9BwSzctP"><img src=".github/raid-discord.png" alt="Join the Retro AI Development (R.A.I.D.) Discord" width="200"></a>
</p>
<!-- /retcomm-readme-raid -->
