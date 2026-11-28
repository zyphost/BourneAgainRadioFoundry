// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Bourne Again RadioFoundry Authors
// The fork's boot splash, BARF (Bourne Again RadioFoundry): a box with a radio, "BARF" and a cursor, the name, the
// codeplug's boot text and the board and version, in DOS EDIT's blue and white (CONFIG_BARF_SPLASH,
// docs/superpowers/specs/2026-11-27-barf-splash-design.md). ui_draw_splash calls it in place of the upstream mark
#ifndef UI_SPLASH_BARF_H
#define UI_SPLASH_BARF_H

#include "config.h"

#if CONFIG_BARF_SPLASH
// Draws the whole screen; the arguments are ui_draw_splash's
void ui_splash_barf(const char *board_name, const char *version, const char *line1, const char *line2);
// The row below the last thing the last call drew above the footer, for the tests: it must not pass the footer's top
int ui_splash_barf_bottom(void);
#endif

#endif
