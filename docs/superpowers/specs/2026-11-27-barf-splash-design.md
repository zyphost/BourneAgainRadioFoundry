<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# BARF boot splash and the fork's name on screen: design

Status: proposed, 2026-11-27. Size: a small change.

## Intent

The fork is Bourne Again RadioFoundry, BARF for short. Its boot screen should say so, as a play on the Bash logo: a
terminal box with a prompt, a radio where the `$` would be, and BARF large. The colors are DOS EDIT's: white and
blue. The owner reviewed three mockups (`spike/barf-splash`, a local throwaway branch) and chose v2: a solid white
box.

## Behavior

The splash, top to bottom, centered above the footer:

| Part | What |
|---|---|
| Box | A solid white box with rounded corners. Inside it the prompt, in blue: the radio with two signal arcs, then **BARF**, then a `_` cursor |
| Name | "Bourne Again RadioFoundry" under the box |
| Rule | A short white line |
| Boot text | The codeplug's two boot text lines, as today. Kept on every display, never dropped |
| Footer | Board name and version, as today |

- **Colors:** background blue `RGB565(0, 0, 170)`, everything else white, the prompt blue on the white box. The splash
  keeps these colors whatever the theme, since it is the fork's mark, like upstream's orange hexagon.
- **Sizes:** the prompt uses the largest font and integer scale whose whole stack fits above the footer. On a short
  display (128x64) the name line and the rule are left out first. The boot text keeps its size, as upstream's splash
  does.
- **Monochrome panels (1 bpp):** the box is ink with the prompt cut out in paper, the rest ink on paper.
- **Every place the splash is drawn:** boot, the stub entry's "radio is being flashed" screen
  (`core/stubentry/stub_entry.c`), and the splash preview in the menus (`core/ui/ui_menus.c`), since they all call
  `ui_draw_splash`.
- The radio glyph is drawn from the shapes of upstream's mark (the handheld and its arcs, on the 1024 grid), so it
  needs no bitmap and scales to every display.

## The name elsewhere (scope A, the owner's default)

| Where | Change |
|---|---|
| The radio's status web page title (`core/net/http_assets/index.html`) | "RadioFoundry" becomes "Bourne Again RadioFoundry" |
| The web control page title (`core/net/webctl_assets/index.html`) | "OpenDigitalRadio" (upstream's legacy name) becomes "Bourne Again RadioFoundry" |
| About, README, FORK.md | Already say it (PR #3) |
| Unchanged | USB, host link, MMDVM, APRS-IS, rigctl, SNMP and mDNS names (tools and protocols read them), the license and copyright lines, `AUTHORS`, upstream's docs and site |

## Build switch

`CONFIG_BARF_SPLASH`, default on for every radio with a display, registered like `CONFIG_STANDBY_BLINK`
(`config/features.mk`, `core/config.h`, `core/hostlink/build_info.c`, `docs/build-config.md`,
`docs/feature-matrix.md`, `tests/hil/features.toml` without a HIL test). Off: upstream's splash, unchanged.

## Fork hygiene (my call)

The BARF drawing goes in a new fork-owned file, `core/ui/ui_splash_barf.c`. Upstream's `ui_draw_splash` gains one
early call under `#if CONFIG_BARF_SPLASH` and is otherwise unchanged, so an upstream change to its splash merges
cleanly. The radio glyph reuses upstream's grid shapes by calling a helper rather than copying them, if that takes
no more than exposing one static function; otherwise the shapes are written again in the new file.

## Constraints

- AGENTS.md: generic core, zero warnings with the switch on and off, US English, SPDX headers.
- Fonts: only those every build has (the layout's fonts and `FONT_LARGEST`); a build without `CONFIG_FONT_XL` uses
  the next size down. Anti-aliased fonts are drawn at scale by thresholding.
- "Bourne Again RadioFoundry" and "BARF" are names, not translated (`TRX` if they go in a table).
- Flash: measure the switch's cost on the owner's two radios. The spike suggests about 1-2 KB.

## Approaches considered

| Approach | Trade-off | Verdict |
|---|---|---|
| Draw it in code from shapes and scaled font glyphs, in a fork-owned file | Scales to every display, small, merges cleanly | **Chosen** |
| A bitmap per display class | Exact pixels, but several KB each and one per class | Dropped |
| Rewrite upstream's `ui_draw_splash` in place | One file, but every upstream splash change conflicts | Dropped |

## Tests

- `uiFlowTest`: the splash on every display class fits (nothing off screen, nothing overlapping the footer), with
  boot text of 0, 1 and 2 lines and with the longest boot text (16 characters). Every pixel is blue, white or a blend
  on color displays.
- `uiSim --layout` passes; the gallery's `boot` and `boot_loader` pages are regenerated (only those, plus the sheets).
- Firmware: the owner's two radios build with zero warnings with the switch on and off.

## Non-goals

- No animation and no blinking cursor (the splash is drawn once).
- No change to the About screen, the themes or the home screen.
