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

#include "settings.h"
#include "../utils.h"
#include "../music.h"
#include "misc_settings.h"
#include <stdlib.h>
#define N(x) scenes_settings_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_SETTINGS_LEN + STR_TOGGLE_TITLES_LEN + STR_REPORT_USER_LEN + STR_LANGUAGE_PICK_LEN + STR_INTEGRATIONS_LEN + STR_SCAN_QR_LEN + STR_SETTINGS_MISC_LEN + STR_BACK_LEN + STR_SYSTEM_LANGUAGE_LEN + STR_LANGUAGE_TOTAL_LEN + STR_BG_MUSIC_LEN + STR_TOGGLE_TITLES_ON_LEN + STR_TOGGLE_TITLES_OFF_LEN)

#define NUM_ENTRIES 8

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_entries[NUM_ENTRIES];
	C2D_Text g_languages[NUM_LANGUAGES + 1];
	C2D_Text g_on_off[2];
	int cursor;
	int selected_language;
	float lang_width;
	float bg_music_width;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	memset(_data, 0, sizeof(N(DataStruct)));
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	_data->cursor = 0;
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_toggle_titles);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_report_user);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_language_pick);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_bg_music);
	TextLangParse(&_data->g_entries[4], _data->g_staticBuf, str_integrations);
	TextLangParse(&_data->g_entries[5], _data->g_staticBuf, str_scan_qr);
	TextLangParse(&_data->g_entries[6], _data->g_staticBuf, str_settings_misc);
	TextLangParse(&_data->g_entries[7], _data->g_staticBuf, str_back);
	TextLangParse(&_data->g_languages[0], _data->g_staticBuf, str_system_language);
	TextLangParse(&_data->g_on_off[0], _data->g_staticBuf, str_toggle_titles_off);
	TextLangParse(&_data->g_on_off[1], _data->g_staticBuf, str_toggle_titles_on);
	_data->selected_language = -1;
	if (config.language != -1) {
		for (int i = 0; i < NUM_LANGUAGES; i++) {
			if (all_languages[i] == config.language) {
				_data->selected_language = i;
				break;
			}
		}
	}
	TextLangSpecificParse(&_data->g_languages[_data->selected_language + 1], _data->g_staticBuf, str_language,
		all_languages[_data->selected_language]);
	get_text_dimensions(&_data->g_entries[2], 1, 1, &_data->lang_width, 0);
	get_text_dimensions(&_data->g_entries[3], 1, 1, &_data->bg_music_width, 0);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < NUM_ENTRIES; i++) {
		C2D_DrawText(&_data->g_entries[i], C2D_AlignLeft, 30, 10 + (i+1)*25, 0, 1, 1);
	}
	C2D_DrawText(&_data->g_languages[_data->selected_language + 1], C2D_AlignLeft, 35 + _data->lang_width, 35 + 50, 0, 1, 1);
	u32 onClr = C2D_Color32(10, 200, 10, 0xff);
	u32 offClr = C2D_Color32(200, 10, 10, 0xff);
	C2D_DrawText(&_data->g_on_off[config.bg_music], C2D_AlignLeft | C2D_WithColor, 35 + _data->bg_music_width, 35 + 75, 0, 1, 1, config.bg_music ? onClr : offClr);
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	int x = 10;
	int y = 10 + (_data->cursor + 1)*25 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 0);
	u32 blue = C2D_Color32(0x2B, 0xCF, 0xFF, 0xFF);
	u32 pink = C2D_Color32(0xF5, 0xAB, 0xB9, 0xFF);
	u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	C2D_DrawRectSolid(400 - 90, 240 - 60, 0, 90, 12, blue);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 12, 0, 90, 12, pink);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 24, 0, 90, 12, white);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 36, 0, 90, 12, pink);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 48, 0, 90, 12, blue);

	C2D_DrawRectSolid(400 - 180, 240 - 60, 0, 90, 10, C2D_Color32(0xE5, 0x00, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 50, 0, 90, 10, C2D_Color32(0xFF, 0x8D, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 40, 0, 90, 10, C2D_Color32(0xFF, 0xEE, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 30, 0, 90, 10, C2D_Color32(0x02, 0x81, 0x21, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 20, 0, 90, 10, C2D_Color32(0x00, 0x4C, 0xFF, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 10, 0, 90, 10, C2D_Color32(0x77, 0x00, 0x88, 0xFF));
}

void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (_data) {
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->cursor < 0) _data->cursor = NUM_ENTRIES - 1;
		if (_data->cursor > NUM_ENTRIES - 1) _data->cursor = 0;
		if (_data->cursor == 2) {
			int old_lang = _data->selected_language;
			_data->selected_language += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT) && 1) - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT) && 1);
			if (_data->selected_language < -1) _data->selected_language = -1;
			if (_data->selected_language > NUM_LANGUAGES-1) _data->selected_language = NUM_LANGUAGES-1;
			if (old_lang != _data->selected_language) {
				config.language = _data->selected_language == -1 ? -1 : all_languages[_data->selected_language];
				configWrite();
				// this if-condition checks if we already parsed the language
				// as we always parse the system language one, if the index is -1 this
				// if-statement will always be false, causing our all_languages index to
				// never get out-of-bounds without explicitly checking it
				if (!*(int*)&_data->g_languages[_data->selected_language + 1]) {
					TextLangSpecificParse(&_data->g_languages[_data->selected_language + 1], _data->g_staticBuf,
						str_language, all_languages[_data->selected_language]);
				}
			}
		}
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				// toggle titles
				sc->next_scene = getToggleTitlesScene();
				return scene_push;
			}
			if (_data->cursor == 1) {
				// report users
				sc->next_scene = getReportListScene();
				return scene_push;
			}
			if (_data->cursor == 3) {
				// bg music
				toggleBgMusic();
				return scene_continue;
			}
			if (_data->cursor == 4) {
				// integrations
				sc->next_scene = getIntegrationScene();
				return scene_push;
			}
			if (_data->cursor == 5) {
				// scan qr
				sc->next_scene = getScanQrScene();
				return scene_push;
			}
			if (_data->cursor == 6) {
				// misc settings
				sc->next_scene = getMiscSettingsScene();
				return scene_push;
			}
			if (_data->cursor == 7) return scene_pop;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getSettingsScene(void) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->is_popup = false;
	scene->need_free = true;
	return scene;
}
