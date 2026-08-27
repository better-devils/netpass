/**
 * NetPass
 * Copyright (C) 2024, 2025 Sorunome
 *               2026 Silentium
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
#include "utils.h"

static u8 _language;

static u8 _gender;

static C2D_Font font_default;

static C2D_Font _cache_fonts_loaded[4] = {0};
static const int fontLoadArr[4] = {CFG_REGION_USA, CFG_REGION_CHN, CFG_REGION_KOR, CFG_REGION_TWN};

C2D_Font getFontIndex(int i) {
	if (_cache_fonts_loaded[i]) {
		return _cache_fonts_loaded[i];
	}
	_cache_fonts_loaded[i] = C2D_FontLoadSystem(fontLoadArr[i]);
	return _cache_fonts_loaded[i];
}

static C2D_Font _get_local_font(int lang) {
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
	int gender_to_set = config.gender;
	if (gender_to_set < 0) {
		gender_to_set = gender_default_map[_language];
	}
	if (gender_to_set < 0) {
		_gender = 2;
		CFLStoreData mii;
		if (R_SUCCEEDED(_e(ACT_GetAccountInfo(&mii, sizeof(CFLStoreData), 0xFE, 0x7)))) {
			_gender = mii.miiData.mii_details.sex ? 1 : 0;
		}
	} else {
		_gender = gender_to_set;
	}
}

const char* _s(LanguageString s) {
	return string_in_language(s, _language);
}

const char* get_text_gender(const char* const* text, u8 g) {
	if (text[g]) return text[g];
	if (g) return get_text_gender(text, g-1);
	return 0;
}

const char* string_in_language(LanguageString s, int lang) {
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		const char* ret;
		if (s[i].language == lang && (ret = get_text_gender(s[i].text, _gender))) {
			return ret;
		}
	}
	return get_text_gender(s[0].text, _gender);
}

// TODO: figure out the other values needed for chinese simplified
static const float font_scale_map[4][4] = {
	{1.f   , 0.866f, 0.866f, 0.9f  },
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

float getFontScale(C2D_Text* text) {
	int local_font_offset = 0;
	int need_font_offset = 0;
	for (int i = 0; i < 4; i++) {
		if (_cache_fonts_loaded[i] == 0) {
			local_font_offset = i;
		}
		if (text->font == _cache_fonts_loaded[i]) {
			need_font_offset = i;
		}
	}
	float scale = font_scale_map[local_font_offset][need_font_offset];
	return scale;
}

C2D_Font _font(LanguageString s) {
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == _language && s[i].text[0]) {
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
		if (s[i].language == l && (text = get_text_gender(s[i].text, _gender))) {
			break;
		}
	}
	C2D_Font font = _get_local_font(l);
	if (!text) {
		text = get_text_gender(s[0].text, _gender);
		font = font_default;
	}
	C2D_TextFontParse(staticText, font, staticBuf, text);
	C2D_TextOptimize(staticText);
}
