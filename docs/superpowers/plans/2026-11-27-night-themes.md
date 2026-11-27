<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# More themes (Night colors, Monochrome) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Night (green, blue, yellow, cyan, pink, orange) and Monochrome themes to Settings > Display > Theme,
behind `CONFIG_UI_EXTRA_THEMES`.

**Architecture:** One palette function makes a Night-style palette from a base color, and one branch makes
Monochrome. Both are called from `ui_theme_set` in `core/ui/ui_screens.c` for the fork's theme numbers 16..22. The
choice is stored in a new settings byte `theme_x`, with the nearest upstream theme kept in `theme` bits 0-1 as the
fallback. The menu, the simulator and the docs list the seven after Night (red).

**Tech Stack:** C11 firmware core (`core/ui`, `core/settings`), host tests in `tests/host` (gfxTest, settingsTest,
uiFlowTest, uiSim), gettext-style catalogs (`core/ui/lang/*.po`, `tools/i18n.py`).

**Spec:** [docs/superpowers/specs/2026-11-27-night-themes-design.md](../specs/2026-11-27-night-themes-design.md)

## Global Constraints

- Theme numbers: `UI_THEME_NIGHT_GREEN` 16, `UI_THEME_NIGHT_BLUE` 17, `UI_THEME_NIGHT_YELLOW` 18,
  `UI_THEME_NIGHT_CYAN` 19, `UI_THEME_NIGHT_PINK` 20, `UI_THEME_NIGHT_ORANGE` 21, `UI_THEME_MONO` 22. Upstream's
  enum (`UI_THEME_DARK`..`UI_THEME_NIGHT`, `UI_THEME_COUNT`) is unchanged.
- Brightest colors (RGB888, written with `RGB565()`): green 0,255,0; blue 80,140,255; yellow 255,230,0; cyan
  0,230,255; pink 255,96,192; orange 255,150,0. The other steps: each channel × 176/255, × 112/255, × 56/255
  (integer, rounded down), the ratios of `NIGHT_R3..R1`.
- Monochrome: background `RGB565(0, 0, 170)`, everything else `RGB565(255, 255, 255)`. No other color.
- Menu labels (English): "Night (green)", "Night (blue)", "Night (yellow)", "Night (cyan)", "Night (pink)",
  "Night (orange)", "Monochrome". Reason on the dimmed Highlight color row: "This theme uses one color".
  Night (red)'s own reason, "Night theme: red only", stays as it is.
- Storage: `uint8_t theme_x` replaces `reserved10[0]` in `settings_t`. 0 = an upstream theme (read `theme` bits 0-1).
  16..22 = that fork theme. The fallback in `theme` bits 0-1: `UI_THEME_NIGHT` (3) for the six Night colors,
  `UI_THEME_DARK` (0) for Monochrome. `sizeof(settings_t)` is unchanged.
- Build switch `CONFIG_UI_EXTRA_THEMES`, default `$(CONFIG_UI_COLOR)`. With it off: no palettes, no menu values; a
  stored `theme_x` reads as its fallback. The byte and the numbers exist either way.
- AGENTS.md: zero warnings with the switch on and off; US English; SPDX headers on new files; every new label in
  es, fr, it and zh (`python3 tools/i18n.py --sync` then `--check`); zh only with glyphs the CJK face has.
- Fork hygiene: touch upstream lines only where the feature needs it; never renumber or recolor upstream themes.
- Never pipe a build or test command through `head` or `tail` when you judge pass or fail: read the whole result line.

## Review Focus

1. **An older record.** `theme_x` 0 with `theme` 3 must still be Night (red). A stored `theme_x` 18 in a build with the
   switch off must read Night (red), its fallback. Pinned in Task 2.
2. **Going back to an upstream theme.** After Night (pink), picking Light must clear `theme_x`, or Pink comes back at the
   next boot. Pinned in Task 2.
3. **The highlight color survives.** A highlight color chosen before a one-color theme must return when Dark is picked
   again. The one-color theme ignores `accent` without changing it. Pinned in Task 1.
4. **Off-color pixels.** Anti-aliased text, the GPS screen, the emergency screen and the pills in every new theme must
   show only shades of the theme's color (Night) or blue, white and their blends (Monochrome), on every color display
   class. Pinned in Task 3 (the generalized `night_checks`).
5. **Indexed 16-color panels** (`240x320i10x5`, `160x120i5`): each theme and its blends draw cleanly there.
   `uiSim --layout` must pass with all eleven themes. Pinned in Task 3.

---

### Task 1: Palettes, theme numbers and the build switch

**Files:**
- Modify: `config/features.mk` (after `CONFIG_UI_COLOR ?= y`, line ~205: `CONFIG_UI_EXTRA_THEMES ?= $(CONFIG_UI_COLOR)`,
  with a comment, and `UI_EXTRA_THEMES` in the FEATURES list), `core/config.h` (default `CONFIG_UI_COLOR`, plus an
  `#error` when on without `CONFIG_UI_COLOR`, as the other sub-features do), `core/hostlink/build_info.c`
  (`features=`), `docs/build-config.md`, `docs/feature-matrix.md` (both tables, bytes measured later in Task 4),
  `tests/hil/features.toml` (an entry with `switch = "UI_EXTRA_THEMES"` and no HIL test).
  The template for every one of these is commit `d8c5bc84` on branch `feat/standby-blink` (`git show d8c5bc84`).
- Modify: `core/ui/ui_screens.h` (the numbers, `bool ui_theme_one_color(uint8_t theme)`),
  `core/ui/ui_screens.c` (`ui_theme_set`, `accent_apply`), `core/ui/ui_gps.c:43`.
- Test: `tests/host/gfxTest.c` (beside the Night block at line ~500).

**Interfaces:**
- Produces: `enum { UI_THEME_NIGHT_GREEN = 16, ..., UI_THEME_MONO = 22 };` (always defined, whatever the switch);
  `bool ui_theme_one_color(uint8_t theme)`: true for `UI_THEME_NIGHT` and, when built, 16..22;
  `uint8_t ui_theme_fallback(uint8_t theme)`: 3 for 16..21, 0 for 22, the theme itself for 0..3.
  `ui_theme_set(t)` accepts 16..22 when `CONFIG_UI_EXTRA_THEMES`; anything else out of range is Dark, as today.

- [ ] **Step 1: Write the failing tests** in `gfxTest.c`, in a new `test_extra_themes()` built `#if CONFIG_UI_EXTRA_THEMES`
  and called from `main`:
  - For each Night color: after `ui_theme_set(t)`, `ui_theme_get() == t`, `ui_col.bg == 0`, `ui_col.fg` equals the
    brightest color, `ui_col.dim == ui_col.rx` equals the 176/255 step, `ui_col.line` the 112/255 step,
    `ui_col.idle == ui_col.hint_bg` the 56/255 step, `ui_col.pill_fg == 0`, `ui_col.idle_fg == ui_col.fg`.
  - For every accent and both strip settings, as the Night block does: `ui_col.focus == ui_col.strip_bg == ui_col.fg`
    and `ui_col.focus_fg == ui_col.strip_warn == ui_col.bg`.
  - Monochrome: every field of `ui_col` is `RGB565(0, 0, 170)` or `RGB565(255, 255, 255)`; `bg` is the blue; `fg`,
    `tx`, `rx`, `focus`, `strip_bg` are white; `pill_fg`, `idle_fg`, `focus_fg` are the blue.
  - `ui_theme_one_color()`: true for 3 and 16..22, false for 0, 1, 2, 4, 15, 23.
  - Review Focus 3: `ui_accent_set(UI_ACCENT_PINK); ui_theme_set(UI_THEME_NIGHT_CYAN); ui_theme_set(UI_THEME_DARK);`
    then `ui_col.focus == RGB565(255, 136, 200)`.
  - `ui_theme_set(23)` is Dark.
- [ ] **Step 2:** `make -C tests/host gfxTest && tests/host/gfxTest`. Expected: it fails to build or the new checks fail.
- [ ] **Step 3: Implement.** In `ui_theme_set`, under `#if CONFIG_UI_EXTRA_THEMES`, before the Night branch: the six Night
  colors go through one static function that fills the roles exactly as the Night branch does, from four colors
  computed from the base. Monochrome is its own short branch. `accent_apply` tests `ui_theme_one_color(ui_theme)`
  instead of `ui_theme == UI_THEME_NIGHT`, and so does `ui_gps.c:43`. Leave the Night (red) branch's code as it is.
- [ ] **Step 4:** Run `gfxTest`; expected PASS. Then `make -C tests/host -j4`: everything passes (the full host suite,
  as the existing night and color tests must still hold).
- [ ] **Step 5:** `make BOARD=dm32 VARIANT=amateur -j4` and `make BOARD=dm32 VARIANT=amateur CONFIG_UI_EXTRA_THEMES=n
  BUILD=build/t-nothemes -j4`: zero warnings each (count `warning:` in the full log). `rm -rf build/t-nothemes`.
- [ ] **Step 6: Commit** "Themes: Night colors and Monochrome palettes, build switch CONFIG_UI_EXTRA_THEMES".

### Task 2: Storage

**Files:**
- Modify: `core/settings/settings.h` (`uint8_t theme_x;` in place of `reserved10[0]`, `reserved10` removed since it
  had one byte; a comment saying the values and the fallback), `core/ui/ui_svc_storage.c` (`UI_SET_THEME` get at
  ~316 and set at ~497), `docs/spec/settings-store.md` (the row for the byte).
- Test: `tests/host/settingsTest.c` (layout table at ~958: `theme_x` at 399, replacing the `reserved10` row), and the
  UI service test that covers `UI_SET_THEME` today (find it with `grep -n UI_SET_THEME tests/host/*.c`; if none
  reads it through `ui_svc_setting_get`, add the checks to `settingsTest.c` through the service functions).

**Interfaces:**
- Consumes: the theme numbers and `ui_theme_fallback()` from Task 1.
- Produces: `ui_svc_setting_get(UI_SET_THEME)` returns 0..3 or, when built, 16..22. `ui_svc_setting_set(UI_SET_THEME,
  v)`: v in 16..22 (when built) stores `theme_x = v` and `theme` bits 0-1 = `ui_theme_fallback(v)`; v in 0..3 stores
  `theme_x = 0` and bits 0-1 = v; any other v clamps as today. Bits 2-7 of `theme` are never touched.

- [ ] **Step 1: Write the failing tests:**
  - The layout row: `offsetof(settings_t, theme_x) == 399`.
  - Set 20 (Night pink): get returns 20; the record has `theme_x == 20` and `theme & 3 == 3`, with bits 2-7 as before.
  - Then set 1 (Light): get returns 1 and `theme_x == 0` (Review Focus 2).
  - Set 22: `theme & 3 == 0`.
  - A record with `theme_x == 0` and `theme == 3` reads 3 (Review Focus 1).
  - Without the switch (the test built with `-DCONFIG_UI_EXTRA_THEMES=0`, or a section under `#if
    !CONFIG_UI_EXTRA_THEMES` in a variant the Makefile already builds without color): a record with `theme_x == 18`
    and `theme == 3` reads 3; setting 18 stores nothing new (clamped to 3, `theme_x` 0).
- [ ] **Step 2:** Build and run the tests touched; expected FAIL.
- [ ] **Step 3: Implement** the field and the get and set.
- [ ] **Step 4:** `make -C tests/host -j4`: all pass.
- [ ] **Step 5: Commit** "Settings: theme_x stores a fork theme, with an upstream fallback in theme".

### Task 3: Menu, translations, simulator and gallery

**Files:**
- Modify: `core/ui/ui_menus.c` (`v_theme`/`n_theme` at ~6610: the seven appended under `#if
  CONFIG_UI_EXTRA_THEMES`; the `UI_SET_ACCENT` reason at ~7527: Night (red) keeps its text, the seven give
  `TR(SET_WHY_ONE_COLOR_THEME, "This theme uses one color")`), `core/ui/lang/{es,fr,it,zh}.po`.
- Modify: `tests/host/sim/ui_sim.h` (`SIM_THEME_COUNT` 11), `tests/host/sim/ui_sim.c` (`sim_themes` gains
  `night_green night_blue night_yellow night_cyan night_pink night_orange mono`, and `sim_theme_find` returns the
  theme's number, 16..22 for the new names, not its index), `tests/host/uiSim.c` (wherever an index is used as a
  theme number; the header comment's theme list), `tests/host/uiFlowTest.c`.
- Regenerate: `make -C tests/host ui-gallery` (about 220 new PNGs: the theme pages in the seven themes on the color
  classes, plus the sheets).

**Interfaces:**
- Consumes: Tasks 1 and 2.
- Keys: `SET_VALUE_THEME_NIGHT_GREEN`, `_NIGHT_BLUE`, `_NIGHT_YELLOW`, `_NIGHT_CYAN`, `_NIGHT_PINK`,
  `_NIGHT_ORANGE`, `SET_VALUE_THEME_MONO`, `SET_WHY_ONE_COLOR_THEME`.

- [ ] **Step 1: Write the failing flows** in `uiFlowTest.c`, in the `#if CONFIG_UI_COLOR` block at ~1341, under
  `#if CONFIG_UI_EXTRA_THEMES`:
  - Picking Night (green) from Night (red) with one more OK: `set7=16`.
  - Picking Monochrome, the last: `set7=22`.
  - The script `theme:night_pink ok 7 2 down down down`, then `ok`: `toast=This_theme set61=0`.
  - Keep `"S theme night, the last"` true where it still is (with the switch off). With the switch on, Night is no
    longer last: rename or guard that flow so each build's flows say what is true.
  - Generalize `night_checks()` (~2236): run the same scripts for every one-color theme. For a Night color, every
    pixel must be black or lie on the line from black to the brightest color: for each channel pair i, j of pixel p
    and base b, `|p_i * b_j - p_j * b_i|` within the RGB565 rounding (find the tolerance from the AA blends; state it
    in a comment). For Monochrome, every pixel lies on the line from the blue to white: red and green equal within
    rounding, blue at least red. Night (red)'s check stays exactly as strict as it is.
- [ ] **Step 2:** `make -C tests/host uiFlowTest && tests/host/uiFlowTest`: the new flows fail.
- [ ] **Step 3: Implement** the menu rows and the reason. Run `python3 tools/i18n.py --sync`. Translate every new key
  into es, fr, it and zh, following the catalogs' existing Night (red) strings ("Noche (rojo)", "Nuit (rouge)",
  "Notte (rosso)", "晚间（红色）"). zh: the CJK face lacks 绿 (green). Use 晚间（森林） for Night (green) unless
  `--check` shows a better word the face can draw; cyan is 青 (as the Teal highlight is), blue 蓝, yellow 黄,
  pink 粉, orange 橙, Monochrome 单色, and the reason 此主题只用一种颜色. Run `python3 tools/i18n.py --check`:
  clean.
- [ ] **Step 4:** Update the simulator (names and numbers), then `uiFlowTest`, `uiFlowTestNoColor` and
  `uiFlowTestSatellite` pass, and `tests/host/uiSim --layout` exits 0.
- [ ] **Step 5:** `make -C tests/host ui-gallery`. Check with `git status --porcelain docs/ux/screens` that only new
  theme pages, the `settings_display` pages and the sheets changed. Open three or four new PNGs (the Read tool) to see
  that they look right: Night (blue) text readable, Monochrome's pills reversed.
- [ ] **Step 6:** `make -C tests/host -j4` all pass. **Commit** "Themes: the menu rows, translations, simulator and
  gallery".

### Task 4: Docs, FORK.md and sizes

**Files:**
- Modify: `docs/user/17-settings.md` (the Theme row lists the seven and says what they are; the Highlight color row
  says it is locked in every one-color theme), `docs/i18n/zh/user/17-settings.md` (same rows, with the labels the
  radio shows), `docs/spec/ui.md` (the themes: the recipe, the numbers, the fallback), `docs/feature-matrix.md` (the
  bytes for `UI_EXTRA_THEMES`, measured on this box as `CONFIG_STANDBY_BLINK`'s were: build each radio with the
  switch on and off and take the `text` difference).
- FORK.md's fork-owned numbers (theme numbers 16..22, `settings_t.theme_x` at offset 399, which was `reserved10[0]`)
  and the `CHANGELOG.md` entry: PR #4 adds FORK.md's "Fork-owned numbers" section and PR #6 adds `CHANGELOG.md`. If
  both are on `main`, run `git merge main` (never rebase) and add the rows and the entry. If not, write them in the
  report as pending, and the controller adds them once those PRs land, so the branches don't conflict.

- [ ] **Step 1:** Write the docs. Run `python3 tools/check_doc_links.py` and `python3 tools/check_generic.py`.
  Expected: 0 broken links. `check_generic` may report FORK.md lines 46-47 (radio names); that predates this branch and
  PR #4 fixes it. Report anything else.
- [ ] **Step 2:** Measure and fill in the bytes. Remove every `build/t-*` tree you made.
- [ ] **Step 3: Commit** "Themes: docs, fork-owned numbers and sizes".

### Task 5 (controller): checks before the PR

- [ ] Both radios build with zero warnings, switch on and off: `make BOARD=dm32 VARIANT=amateur`, `make
  BOARD=mduv380`, each again with `CONFIG_UI_EXTRA_THEMES=n BUILD=build/t-x`. Then `rm -rf build/t-*`.
- [ ] `make -C tests/host -j4` passes.
- [ ] The full `tools/ci.sh` waits for the end of this phase of work (the owner's choice), with the venv on PATH.
- [ ] Push and open a fork PR, through superpowers:finishing-a-development-branch. Never merge.
