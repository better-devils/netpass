/**
 * NetPass
 * Copyright (C) 2024 SunOfLife1
 *               2025 Sorunome
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "render.h"
#include "utils.h"

C2D_SpriteSheet spr_cursor = 0;

u32 clr_white;
u32 clr_gray;
u32 clr_black;
u32 clr_netpass_green;
u32 clr_focus_blue;
u32 clr_off_red;

void renderInit(void) {
	spr_cursor = C2D_SpriteSheetLoad("romfs:/gfx/cursor.t3x");
	
	clr_white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	clr_gray = C2D_Color32(0x4c, 0x4c, 0x4c, 0xFF);
	clr_black = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
	clr_netpass_green = C2D_Color32(0x09, 0x65, 0x1e, 0xFF);
	clr_focus_blue = C2D_Color32(0x08, 0xB4, 0xC4, 0xFF);
	clr_off_red = C2D_Color32(0xC8, 0x0A, 0x0A, 0xFF);
}

void renderExit(void) {
	if (spr_cursor) C2D_SpriteSheetFree(spr_cursor);
}

void renderTextWithOutline(C2D_Text* text, u32 flags, float x, float y, float z, float scaleX, float scaleY, float outlineWidth, u32 textClr, u32 outlineClr, ...) {
	if (y > SCREEN_TOP_HEIGHT) return;
	
	// I hate this so much this is so stupid
	// ^ soru agrees to that sentiment. sadly she had to make it even worse for the outlines she wants

	float xPos = x + outlineWidth;
	float xNeg = x - outlineWidth;
	float yPos = y + outlineWidth;
	float yNeg = y - outlineWidth;

	va_list args;
	va_start(args, outlineClr);

	// Outline
	int steps = 3 + outlineWidth;
	float stepSize = outlineWidth * 2. / steps;
	// We want to do as few C2D_DrawText calls as possible, so we deduplicate with
	// the rounded (x, y) coordinates.
	u32 had_x[steps];
	u32 had_y[steps];
	int ii = 0;
	for (float i = xNeg; i <= xPos; i += stepSize, ii++) {
		bool found = false;
		for (int k = 0; k < ii; k++) {
			if (had_x[k] == ii) {
				found = true;
				break;
			}
		}
		had_x[ii] = round(i);
		if (found) continue;
		int jj = 0;
		for (float j = yNeg; j <= yPos; j += stepSize, jj++) {
			if (i != xNeg && i != xPos && j != yNeg && j != yPos) continue;
			if (round(i) == round(x) && round(j) == round(y)) continue;
			bool found = false;
			for (int k = 0; k < jj; k++) {
				if (had_y[k] == jj) {
					found = true;
					break;
				}
			}
			had_y[jj] = round(j);
			if (found) continue;
			C2D_DrawText(text, C2D_WithColor | flags, i, j, z, scaleX, scaleY, outlineClr, args);
		}
	}
	
	// Actual text
	C2D_DrawText(text, C2D_WithColor | flags, x, y, z, scaleX, scaleY, textClr, args);

	va_end(args);
}

void renderText(C2D_Text* text, float x, float y, float scale, u32 clr) {
	renderTextWithOutline(text, C2D_AlignLeft, x, y, 0, scale, scale, scale * 3., clr_white, clr ? clr : clr_netpass_green);
}

void renderTextFlags(C2D_Text* text, u32 flags, float x, float y, float scale, u32 clr, ...) {
	va_list args;
	va_start(args, clr);
	renderTextWithOutline(text, flags, x, y, 0, scale, scale, scale * 3., clr_white, clr ? clr : clr_netpass_green, args);
	va_end(args);
}

void renderCursor(float x, float y, float scale) {
	if (!spr_cursor) return;
	C2D_Image img = C2D_SpriteSheetGetImage(spr_cursor, 0);
	C2D_DrawImageAt(img, x, y, 0, NULL, scale, scale);
}
