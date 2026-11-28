// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Bourne Again RadioFoundry Authors
// The fork's boot splash (ui_splash_barf.h). A play on a shell's prompt: a white box holding a radio where the "$"
// would be, "BARF" and a cursor, in blue. Everything is drawn from shapes and scaled font glyphs, so it fits every
// display class with no bitmap. The layout takes the largest font and scale whose whole stack fits above the footer;
// on a short display the name line, then the rule, go first, and the boot text is never dropped
#include "ui_splash_barf.h"

#if CONFIG_BARF_SPLASH
#include "ui_screens.h"
#include "gfx.h"
#include "font.h"
#include "fmt.h"

// DOS EDIT's blue and white: the only two colors (the splash keeps them whatever the theme, as upstream's mark keeps
// its orange)
#define BARF_BLUE	RGB565(0, 0, 170)
#define BARF_WHITE	RGB565(255, 255, 255)

static int barf_bottom;

static bool in_rrect(int px, int py, int x, int y, int w, int h, int r)
{
	if (px < x || px >= x + w || py < y || py >= y + h)
		return false;
	int cx = px < x + r ? x + r : px >= x + w - r ? x + w - r - 1 : px;
	int cy = py < y + r ? y + r : py >= y + h - r ? y + h - r - 1 : py;
	return (px - cx) * (px - cx) + (py - cy) * (py - cy) <= r * r;
}

// The handheld of upstream's mark on its 1024 grid (antenna, side knob, body; the display, speaker slits and key cut
// out) and two signal arcs beside the antenna, within 42 degrees of horizontal. True where it is ink
static bool radio_ink(int X, int Y)
{
	if (in_rrect(X, Y, 434, 193, 37, 311, 18) || in_rrect(X, Y, 553, 441, 46, 74, 14) ||
	    in_rrect(X, Y, 397, 482, 230, 370, 46)) {
		if (in_rrect(X, Y, 448, 532, 129, 63, 7))
			return false;
		for (int i = 0; i < 3; i++)
			if (in_rrect(X, Y, 436, 652 + 37 * i, 152, 15, 7))
				return false;
		return (X - 512) * (X - 512) + (Y - 800) * (Y - 800) > 23 * 23;
	}
	int dx = X - 452, dy = Y - 286, d2 = dx * dx + dy * dy, ady = dy < 0 ? -dy : dy, adx = dx < 0 ? -dx : dx;
	if (ady * 100 > adx * 90)
		return false;
	static const short ring[2] = { 90, 160 };	// radii; each arc 30 wide
	for (int i = 0; i < 2; i++)
		if (d2 >= (ring[i] - 15) * (ring[i] - 15) && d2 <= (ring[i] + 15) * (ring[i] + 15))
			return true;
	return false;
}

// The radio in a w x h box: the grid's X 262..642 and Y 159..852, which hold the arcs and the body
static void draw_radio(int x, int y, int w, int h, color_t c)
{
	for (int py = 0; py < h; py++)
		for (int px = 0; px < w; px++)
			if (radio_ink(262 + (2 * px + 1) * 380 / (2 * w), 159 + (2 * py + 1) * 693 / (2 * h)))
				gfx_pixel(x + px, y + py, c);
}

// Text s times larger: each font pixel an s x s block. An anti-aliased (2 bpp) glyph keeps its darker half, so the
// letters stay two-color. Returns the x after the text
static int text_scaled(const font_t *f, int x, int y, const char *t, int s, color_t c)
{
	int stride = (f->w * f->bpp + 7) / 8;
	for (; *t != '\0'; t++, x += f->w * s) {
		int top, rows;
		const uint8_t *g = font_glyph(f, (uint8_t)*t, &top, &rows);
		for (int r = 0; g != NULL && r < rows; r++) {
			const uint8_t *row = g + r * stride;
			for (int col = 0; col < f->w; col++) {
				int v = f->bpp == 1 ? (row[col / 8] >> (7 - col % 8)) & 1 :
					((row[col / 4] >> (6 - 2 * (col % 4))) & 3) >= 2;
				if (v)
					gfx_fill(x + col * s, y + (top + r) * s, s, s, c);
			}
		}
	}
	return x;
}

int ui_splash_barf_bottom(void)
{
	return barf_bottom;
}

void ui_splash_barf(const char *board_name, const char *version, const char *line1, const char *line2)
{
	static const char name[] = "Bourne Again RadioFoundry";
	int w = gfx_width(), h = gfx_height();
	const ui_layout_t *L = &ui_layout;
	bool mono = gfx_bpp() == 1;
	color_t bg = mono ? ui_col.bg : BARF_BLUE, fg = mono ? ui_col.fg : BARF_WHITE;
	gfx_clear(bg);

	// The owner's lines (the CPS boot text, or the stub entry's): as large as fits, never dropped
	const char *lines[2] = { line1 != NULL ? line1 : "", line2 != NULL ? line2 : "" };
	int n_lines = (lines[0][0] != '\0') + (lines[1][0] != '\0');
	const font_t *lf = L->medium;
	for (int i = 0; i < 2; i++)
		if (gfx_text_width(lf, lines[i]) > w - 2)
			lf = L->small;

	// Footer: board and version along the bottom; too wide, one space between them, then the version alone
	const font_t *ff = L->tiny;
	char foot[48];
	static const char *const seps[] = { "  ", " ", NULL };
	for (int i = 0; i < 3; i++) {
		sbuf_t b;
		sb_init(&b, foot, sizeof foot);
		if (seps[i] != NULL) {
			sb_str(&b, board_name);
			sb_str(&b, seps[i]);
		}
		sb_str(&b, version);
		if (gfx_text_width(ff, foot) <= w)
			break;
	}
	int foot_y = h - ff->h;
	int gap = h >= 200 ? 8 : h >= 100 ? 4 : 2;
	const font_t *nf = gfx_text_width(L->medium, name) <= w - 6 ? L->medium :
		gfx_text_width(L->small, name) <= w - 4 ? L->small : L->tiny;

	// The prompt: the radio (1.2 cells), a 0.3 cell gap, BARF, the cursor; 6.6 cells. The largest font and scale whose
	// stack fits, BARF's size first: at each size with the name and the rule, then without them. The box keeps a margin
	// of a twelfth of the width
	const font_t *const fonts[] = { L->large, L->medium, L->small };
	const font_t *pf = L->small;
	int ps = 1, cw = 0, ch = 0, pad = 2, box_w = 0, box_h = 0, total = 0;
	bool show_name = false, found = false;
	for (int sc = 4; sc >= 1 && !found; sc--) {
		for (size_t i = 0; i < sizeof fonts / sizeof fonts[0] && !found; i++) {
			for (int pass = 0; pass < 2 && !found; pass++) {
				bool with_name = pass == 0;
				cw = fonts[i]->w * sc;
				ch = fonts[i]->h * sc;
				pad = cw / 3 > 2 ? cw / 3 : 2;
				box_w = cw * 66 / 10 + 2 * pad;
				box_h = ch + 2 * pad;
				total = box_h + (with_name ? gap + nf->h : 0) +
					(n_lines > 0 ? (with_name ? 2 * gap : gap) + n_lines * lf->h : 0);
				if (box_w <= w - w / 12 && ch <= h * 45 / 100 && total <= foot_y - 2) {
					pf = fonts[i];
					ps = sc;
					show_name = with_name;
					found = true;
				}
			}
		}
	}
	if (!found) {		// nothing fits (no display class today): the smallest, without the name
		cw = pf->w;
		ch = pf->h;
		pad = 2;
		box_w = cw * 66 / 10 + 2 * pad;
		box_h = ch + 2 * pad;
		total = box_h + (n_lines > 0 ? gap + n_lines * lf->h : 0);
	}

	int y = (foot_y - total) / 2;
	if (y < 0)
		y = 0;
	int bx = (w - box_w) / 2;
	gfx_round_fill(bx, y, box_w, box_h, fg);
	int px = bx + pad, py = y + pad;
	int rw = cw * 12 / 10, rh = ch * 9 / 10;
	draw_radio(px, py + (ch - rh) / 2, rw, rh, bg);
	int tx = text_scaled(pf, px + rw + cw * 3 / 10, py, "BARF", ps, bg);
	int uh = ch / 12 > 2 ? ch / 12 : 2;	// the cursor: an underscore on the baseline
	gfx_fill(tx + cw / 8, py + pf->ascent * ps - uh / 2 + ps, cw * 3 / 4, uh, bg);
	y += box_h;
	if (show_name) {
		y += gap;
		gfx_text(nf, (w - gfx_text_width(nf, name)) / 2, y, name, fg);
		y += nf->h;
	}
	if (n_lines > 0) {
		y += gap;
		if (show_name) {
			gfx_hline(w / 2 - w / 6, y, w / 3, fg);
			y += gap;
		}
		for (int i = 0; i < 2; i++) {
			if (lines[i][0] == '\0')
				continue;
			gfx_text(lf, (w - gfx_text_width(lf, lines[i])) / 2, y, lines[i], fg);
			y += lf->h;
		}
	}
	barf_bottom = y;
	gfx_text(ff, (w - gfx_text_width(ff, foot)) / 2, foot_y, foot, fg);
}
#endif
