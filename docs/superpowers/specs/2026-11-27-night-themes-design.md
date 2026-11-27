<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# More themes: Night colors and Monochrome: design

Status: approved by the owner, 2026-11-27. Size: a small change.

## Intent

The owner likes the Night (red) theme and wants the same single-color look in other colors. He also wants a
two-color theme in the style of DOS EDIT: white on blue. These themes are a matter of taste. Only red keeps eyes
adapted to the dark, and the owner chose the "Night" names knowing that.

## Behavior

Seven themes join Dark, Light, High contrast and Night (red). Settings > Display > Theme lists them after Night (red),
in this order:

| Theme (label) | Brightest color (RGB) |
|---|---|
| Night (green) | 0, 255, 0 |
| Night (blue) | 80, 140, 255 |
| Night (yellow) | 255, 230, 0 |
| Night (cyan) | 0, 230, 255 |
| Night (pink) | 255, 96, 192 |
| Night (orange) | 255, 150, 0 |
| Monochrome | white 255, 255, 255 on blue 0, 0, 170 |

- **Night colors.** These follow Night (red)'s recipe exactly (`ui_screens.c`, `NIGHT_R1..R4`): four steps of one
  color on black. The brightest step is the color in the table. The others are that color scaled by 176/255, 112/255
  and 56/255, the ratios Night (red) uses. Each role keeps Night (red)'s step: text, TX and warnings brightest; RX,
  privacy and dim text one step down; lines two down; the idle pill and hint row darkest. Night (blue)'s brightest
  step is a sky blue because pure blue on black is 2.4:1, too dim to read.
- **Monochrome.** Background blue `0, 0, 170`, everything else white. There is no gray and no third color. The focus
  row, the filled status strip and the TX, RX and idle pills are reverse video: blue text on a white bar. TX and RX
  differ by their words, as in Night (red).
- **The rules Night (red) has, for all seven.** The Highlight color setting is locked to the theme's own color and
  shows why: one reason text for the whole family ("This theme uses one color"), replacing the red-only one. The
  filled status strip is the brightest step (white in Monochrome) with background-colored text. The GPS screen's
  per-system colors are off (`ui_gps.c`).
- The black and white displays are unchanged: their theme rows stay dimmed, as today.

## Storage (my call)

`settings_t.theme` keeps 0..3 in bits 0-1. Its other bits hold the crash report's target, so it has no room.

- A new byte `theme_x` takes `reserved10[0]` (`core/settings/settings.h`). Its value is 0 for an upstream theme, or
  the fork's theme number (below). An older record reads 0, so it keeps its theme. The byte is a different line from
  the one PR #4 (standby blink) takes from `reserved11`, so the two branches don't conflict.
- Choosing a fork theme also writes its nearest upstream theme into `theme` bits 0-1: Night (red) for the six Night
  colors, Dark for Monochrome. A reflash to upstream firmware then shows a theme close to the one chosen.
- **Theme numbers.** `UI_THEME_*` values 16..22 are the fork's, in the table's order. Upstream's next theme would be
  4, so no stored value or menu value ever collides. The numbers go in FORK.md's fork-owned numbers.

## Build switch

`CONFIG_UI_EXTRA_THEMES`, registered like `CONFIG_STANDBY_BLINK`: `config/features.mk` (FEATURES), `core/config.h`,
`core/hostlink/build_info.c`, `docs/build-config.md`, `docs/feature-matrix.md` and `tests/hil/features.toml` (no
HIL test: the theme is a screen color, which the host tests and gallery check).

- Default on wherever `CONFIG_UI_COLOR` is on. Off: no new rows and no palettes. A stored fork theme reads as its
  fallback in `theme` bits 0-1. The `theme_x` byte and the theme numbers stay either way.
- Expected cost: well under 1 KB of code, plus the translated labels.

## Constraints

- AGENTS.md hard rules: one stack, zero warnings in every `tools/ci.sh` configuration with the switch on and off, US
  English, an SPDX header on new files, every label translated (es, fr, it, zh), within the CJK face's glyphs.
- Fork hygiene: the diff stays small, and upstream's themes, numbers and stored values are unchanged.
- The indexed 16-color panels: a theme uses at most 6 colors, which fits like Night (red).

## Approaches considered

| Approach | Trade-off | Verdict |
|---|---|---|
| A palette function for the Night family that takes one base color and scales it, plus one Monochrome branch | Smallest code, one recipe, and Night (red) can use it too without changing its colors | **Chosen** |
| A full `ui_colors_t` table per theme | Easy to tweak each, but six near-copies cost flash and drift apart | Dropped |
| Widen `theme` to more bits | It has no room: the crash report's target owns bits 2-7 | Dropped |

## Tests

- Host (`tests/host/gfxTest.c`, beside the Night tests): each new theme's colors after `ui_theme_set`, including the
  scaled steps and Night (blue)'s brightest. The Highlight color can't change a single-color theme. An out-of-range
  number reads Dark.
- Settings (`tests/host/settingsTest.c` and the UI service): a fork theme stores `theme_x` and its fallback; an older
  record (`theme_x` 0) keeps its theme; with the switch off, a stored fork theme reads its fallback.
- UI: `uiFlowTest` flows for picking Night (green) and Monochrome, and the dimmed Highlight color with its reason. The
  simulator's theme list (`sim_themes`) gains the seven, so `uiSim --layout` checks every screen in them, and the
  gallery draws the theme pages in each (about 220 new PNGs).
- A `radioCoreTest`-style variant isn't needed: the themes live in the UI only. `uiFlowTestNoColor` covers the
  build without color.
- `make BOARD=<b> CONFIG_UI_EXTRA_THEMES=n` builds clean on the owner's two radios.

## Docs

`docs/user/17-settings.md` (the Theme row and the Highlight color note), its zh copy, `docs/spec/ui.md` (themes),
`docs/spec/settings-store.md` (the `theme_x` row), `docs/build-config.md`, `docs/feature-matrix.md`, FORK.md (the
theme numbers and `theme_x`), and a CHANGELOG.md entry.

## Non-goals

- No change to Dark, Light, High contrast or Night (red).
- No user-defined colors.
- No theme for the black and white displays.
