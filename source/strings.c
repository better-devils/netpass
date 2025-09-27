/**
 * NetPass
 * Copyright (C) 2024, 2025 Sorunome
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

#include "strings.h"
#include "config.h"

static u8 _language;

C2D_Font font_default;

C2D_Font _cache_fonts_loaded[4] = {0};
const int fontLoadArr[4] = {CFG_REGION_USA, CFG_REGION_CHN, CFG_REGION_KOR, CFG_REGION_TWN};

C2D_Font getFontIndex(int i) {
	if (_cache_fonts_loaded[i]) {
		return _cache_fonts_loaded[i];
	}
	_cache_fonts_loaded[i] = C2D_FontLoadSystem(fontLoadArr[i]);
	return _cache_fonts_loaded[i];
}

C2D_Font _get_local_font(int lang) {
	C2D_Font font;
	if (lang == CFG_LANGUAGE_ZH) {
		font = getFontIndex(1);
	} else if (lang == CFG_LANGUAGE_KO) {
		font = getFontIndex(2);
	} else if (lang == CFG_LANGUAGE_TW) {
		font = getFontIndex(3);
	} else {
		font = getFontIndex(0);
	}
	return font;
}

void stringsInit(void) {
	if (config.language == -1) {
		CFGU_GetSystemLanguage(&_language);
	} else {
		_language = config.language;
	}
	font_default = getFontIndex(0);
}

const char* _s(LanguageString s) {
	return string_in_language(s, _language);
}

const char* string_in_language(LanguageString s, int lang) {
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == lang && s[i].text) {
			return s[i].text;
		}
	}
	return s[0].text;
}

// TODO: figure out the other values needed for chinese simplified
static const float font_scale_map[4][4] = {
	{1.f   , 1.145f, 1.145f, 1.5f  },
	{0.866f, 1.f   , 1.f   , 1.299f},
	{0.866f, 0.925f, 1.f   , 1.299f},
	{0.666f, 0.715f, 0.768f, 1.f   },
};
void get_text_dimensions(C2D_Text* text, float scale_x, float scale_y, float* width, float* height) {
	C2D_TextGetDimensions(text, scale_x, scale_y, width, height);
	// got local font, nothing to do
	if (!text->font) return;
	int local_font_offset = 0;
	int need_font_offset = 0;
	for (int i = 0; i < 4; i++) {
		getFontIndex(i);
		if (_cache_fonts_loaded[i] == 0) {
			local_font_offset = i;
		}
		if (text->font == _cache_fonts_loaded[i]) {
			need_font_offset = i;
		}
	}
	float scale = font_scale_map[local_font_offset][need_font_offset];
	if (width) {
		*width = *width * scale;
	}
}

C2D_Font _font(LanguageString s) {
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == _language && s[i].text) {
			return _get_local_font(_language);
		}
	}
	return font_default;
}

u8 get_nintendo_language(void) {
	return _language > NUM_NINTENDO_LANGUAGES ? CFG_LANGUAGE_EN : _language;
}

u8 get_language(void) {
	return _language;
}

void TextLangParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s) {
	TextLangSpecificParse(staticText, staticBuf, s, _language);
}

void TextLangSpecificParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s, int l) {
	const char* text = 0;
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == l && s[i].text) {
			text = s[i].text;
			break;
		}
	}
	C2D_Font font = _get_local_font(l);
	if (!text) {
		text = s[0].text;
		font = font_default;
	}
	C2D_TextFontParse(staticText, font, staticBuf, text);
	C2D_TextOptimize(staticText);
}
