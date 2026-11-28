<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# BARF boot splash Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the boot splash with the BARF mark (v2: a white box with a blue radio, BARF and a cursor, on DOS
blue) behind `CONFIG_BARF_SPLASH`, and put the fork's name in the radio's two web page titles.

**Architecture:** A new fork-owned file `core/ui/ui_splash_barf.c` draws the whole splash. Upstream's
`ui_draw_splash` (`core/ui/ui_screens.c`) gains one early call under `#if CONFIG_BARF_SPLASH`. The radio glyph is
drawn per pixel from rounded-rectangle shapes on a 1024 grid, and BARF is a font scaled up by drawing each glyph
pixel as a block. The layout picks the largest font and scale whose whole stack fits above the footer.

**Tech Stack:** C11 (`core/ui`), host tests in `tests/host` (uiFlowTest, uiSim), make (`config/features.mk`).

**Spec:** [docs/superpowers/specs/2026-11-27-barf-splash-design.md](../specs/2026-11-27-barf-splash-design.md)

## Global Constraints

- Colors: blue `RGB565(0, 0, 170)`, white `RGB565(255, 255, 255)`, nothing else; the prompt (radio, BARF, `_`) is
  blue on the white box. On 1 bpp panels: the box is ink, the prompt paper, the rest ink on paper.
- Text: "BARF" and "Bourne Again RadioFoundry" are names, never translated. The boot text lines and the footer
  (board name, two spaces, version; the version alone when too wide) as upstream draws them.
- The boot text is never dropped. On a short display the name line, then the rule, go first.
- Switch `CONFIG_BARF_SPLASH`, default `y`. Off: upstream's splash, byte for byte.
- AGENTS.md: zero warnings on and off; generic core; SPDX headers; US English; only the fonts every build has.
- Never pipe a build or test through `head` or `tail` when judging pass or fail.

## Review Focus

1. **The longest boot text** (16 wide characters, two lines) on 128x64 and 160x120: nothing overlaps the footer.
   Pinned in Task 1.
2. **No boot text** (the owner's radio): the stack is centered without a gap where the lines would be. Pinned in
   Task 1.
3. **A build without the 16x32 font** (`CONFIG_FONT_XL=n`): the next font down is used, no missing symbol. Pinned in
   Task 2's builds.
4. **The stub entry's splash** (`splash:loader`): its two lines are the tool's text, longer than boot text. Pinned in
   Task 1 (gallery `stub_entry` and the fit check).
5. **Off-color pixels** on color panels: only blue, white and blends between them. Pinned in Task 1.

---

### Task 1: The splash

**Files:** Create `core/ui/ui_splash_barf.c`, `core/ui/ui_splash_barf.h`. Modify `core/ui/ui_screens.c`
(`ui_draw_splash`: the early call), `config/features.mk`, `core/config.h`, `core/hostlink/build_info.c`,
`tests/hil/features.toml` (no HIL test), the firmware source lists that need the new file (find how `ui_screens.c`
is listed: `grep -rn ui_screens.c Makefile platform/*.mk tests/host/Makefile`), `tests/host/uiFlowTest.c`.
Regenerate the gallery.

**Interfaces:** `void ui_splash_barf(const char *board_name, const char *version, const char *line1, const char
*line2)`; for tests, `int ui_splash_barf_bottom(void)`: the last row the stack used in the last call.

- [ ] Write `splash_barf_checks()` in uiFlowTest (built `#if CONFIG_BARF_SPLASH`): for every display class, with
  boot text of none, one line, two lines and two 16-character lines, and for `splash:loader`: the stack's bottom is
  above the footer row; on color classes every pixel is blue, white or on the line between them (the tolerance of
  the themes branch: one RGB565 step); the box's white is present.
- [ ] Run it: fails (no function).
- [ ] Implement the file, the hook and the switch. Build uiFlowTest: passes; `uiSim --layout` 0 problems.
- [ ] `make -C tests/host ui-gallery`: only `boot`, `stub_entry` and the sheets change. Look at three of them.
- [ ] `make -C tests/host -j4` all pass. Commit.

### Task 2: Web titles, docs, sizes

**Files:** `core/net/http_assets/index.html` and `core/net/webctl_assets/index.html` (`<title>`), `docs/user/03-first-boot.md`
(the boot screen paragraph: the BARF mark in a build with `CONFIG_BARF_SPLASH`), `docs/spec/ui.md` (the `ui.c`
splash row), `docs/build-config.md`, `docs/feature-matrix.md` (bytes, measured switch on vs off on the owner's two
radios).

- [ ] Edit; `python3 tools/check_doc_links.py`, `python3 tools/check_generic.py` (only FORK.md:46,47 expected).
- [ ] Both radios build with zero warnings, switch on and off; one radio also with `CONFIG_FONT_XL=n`. Remove
  `build/t-*`. Commit.
- [ ] One whole-branch review (most capable model); fix what it finds; push; fork PR. Never merge.
