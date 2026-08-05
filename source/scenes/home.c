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

#include "home.h"
#include <stdlib.h>
#include "../api.h"
#include "../config.h"
#include "../image_cache.h"
#include "../render.h"
#include "../music.h"
#include "scan_qr.h"
#define _data ((DataStruct*)sc->d)
#define TEXT_BUF_LEN (STR_AT_HOME_LEN + STR_GOTO_TRAIN_STATION_LEN + STR_GOTO_PLAZA_LEN + STR_GOTO_MALL_LEN + STR_GOTO_BEACH_LEN + STR_GOTO_ARCADE_LEN + STR_GOTO_CATCAFE_LEN + STR_SCAN_QR_LEN + STR_SETTINGS_LEN + STR_EXIT_LEN)

#define NUM_ENTRIES (NUM_LOCATIONS + 3)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_home;
	C2D_Text g_entries[NUM_ENTRIES];
	C2D_Text artist;
	C2D_Image background;
	C2D_SpriteSheet spr;
	int cursor;
	bool have_artist;
	bool view_bg_only;
} DataStruct;

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	memset(sc->d, 0, sizeof(DataStruct));
	get_background_image("home", &_data->spr, &_data->background, true);
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + 50);
	_data->cursor = 0;
	_data->view_bg_only = false;
	_data->have_artist = false;
	TextLangParse(&_data->g_home, _data->g_staticBuf, str_at_home);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_goto_train_station);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_goto_plaza);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_goto_mall);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_goto_beach);
	TextLangParse(&_data->g_entries[4], _data->g_staticBuf, str_goto_arcade);
	TextLangParse(&_data->g_entries[5], _data->g_staticBuf, str_goto_catcafe);
	TextLangParse(&_data->g_entries[6], _data->g_staticBuf, str_scan_qr);
	TextLangParse(&_data->g_entries[7], _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[8], _data->g_staticBuf, str_exit);
	
	if (*location.artist_name) {
		char string[150];
		snprintf(string, 150, _s(str_artist_copyright), location.artist_name);
		C2D_TextFontParse(&_data->artist, _font(str_artist_copyright), _data->g_staticBuf, string);
		_data->have_artist = true;
	}
}

static void render(Scene* sc) {
	if (!_data) return;
	if (_data->spr) {
		C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, 0);
		C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
	} else if (_data->background.tex) {
		C2D_DrawImageAt(_data->background, 0, 0, 0, NULL, 1, 1);
	}
	if (_data->view_bg_only) return;
	
	if (_data->have_artist) {
		renderTextFlags(&_data->artist, C2D_AlignRight, SCREEN_TOP_WIDTH - 2, SCREEN_TOP_HEIGHT - 12, 0.4, 0);
	}
	
	renderText(&_data->g_home, 10, 10, 1, 0);
	
	for (int i = 0; i < NUM_ENTRIES; i++) {
		renderText(&_data->g_entries[i], 30, 35 + 5 + i*14, 0.5, config.last_location == i ? clr_gray : 0);
		if (config.last_location == i) {
			float width;
			get_text_dimensions(&_data->g_entries[i], 0.5, 0.5, &width, 0);
			int y = 35 + 5 + i*14 + 8;
			C2D_DrawLine(30, y, clr_gray, 30 + width, y, clr_gray, 2, 0);
		}
	}
	renderCursor(13, 35 + 5 + _data->cursor*14 + 1, 0.5);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		if (_data->spr) {
			C2D_SpriteSheetFree(_data->spr);
		}
		if (_data->background.tex) {
			C2D_ImageDelete(&_data->background);
		}
		free(_data);
	}
}

static s32 new_location;

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	if (_data) {
		_data->view_bg_only = (kHeld & KEY_L) || (kHeld & KEY_R);
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->cursor < 0) _data->cursor = (NUM_ENTRIES-1);
		if (_data->cursor > (NUM_ENTRIES-1)) _data->cursor = 0;
		if (kDown & KEY_A) {
			if (_data->cursor == NUM_ENTRIES-3) {
				sc->next_scene = getScanQrScene();
				return scene_push;
			}
			if (_data->cursor == NUM_ENTRIES-2) {
				sc->next_scene = getSettingsScene();
				return scene_push;
			}
			if (_data->cursor == NUM_ENTRIES-1) return scene_stop;
			// load location scene
			if (_data->cursor == config.last_location) {
				sc->next_scene = getInfoScene(str_no_location_twice);
				return scene_push;
			}
			new_location = _data->cursor;
			// we do not need to actually switch scenes to the location scene here, as our other code will do that for us
			sc->next_scene = getLoadingScene(NULL, lambda(void, (void) {
				Result res = _e(setLocation(new_location));
				if (!R_FAILED(res)) {
					_e(getLocation());
				}
				triggerDownloadInboxes();
			}));
			return scene_push;
		}
		if (location.id != -1) {
			sc->next_scene = getLocationScene(location.id);
			return scene_switch;
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getHomeScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
