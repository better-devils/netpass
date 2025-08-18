/**
 * NetPass
 * Copyright (C) 2024-2025 Sorunome
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
#include <3ds.h>

#include <citro2d.h>
#include "../codegen/lang_strings.h"

void stringsInit(void);
const char* _s(LanguageString s);
const char* string_in_language(LanguageString s, int lang);
void get_text_dimensions(C2D_Text* text, float scale_x, float scale_y, float* width, float* height);
C2D_Font _font(LanguageString s);
C2D_Font getFontIndex(int i);
u8 get_nintendo_language(void);
u8 get_language(void);
void TextLangParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s);
void TextLangSpecificParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s, int l);
