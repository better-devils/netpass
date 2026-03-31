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

#pragma once

#include <citro2d.h>
#include <3ds.h>

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240

#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_BOTTOM_HEIGHT 240

#define CENTER_TOP_X(img_width) ((SCREEN_TOP_WIDTH - img_width) / 2)
#define CENTER_TOP_Y(img_height) ((SCREEN_TOP_HEIGHT - img_height) / 2)

#define CENTER_BOTTOM_X(img_width) ((SCREEN_BOTTOM_WIDTH - img_width) / 2)
#define CENTER_BOTTOM_Y(img_height) ((SCREEN_BOTTOM_HEIGHT - img_height) / 2)

extern u32 clr_white;
extern u32 clr_gray;
extern u32 clr_black;
extern u32 clr_netpass_green;
extern u32 clr_focus_blue;
extern u32 clr_off_red;

extern u8 fade_alpha;

void renderInit(void);
void renderExit(void);

void renderTextWithOutline(C2D_Text* text, u32 flags, float x, float y, float z, float scaleX, float scaleY, float outlineWidth, u32 textClr, u32 outlineClr, ...);
void renderPlainText(C2D_Text* text, float x, float y, float scale, u32 clr);
void renderPlainTextFlags(C2D_Text* text, u32 flags, float x, float y, float scale, u32 clr, ...);
void renderText(C2D_Text* text, float x, float y, float scale, u32 clr);
void renderTextFlags(C2D_Text* text, u32 flags, float x, float y, float scale, u32 clr, ...);
void renderCursor(float x, float y, float scale);
void renderBottomScreen(void);
