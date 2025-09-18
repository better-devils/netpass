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

#include "home.h"
#include "switch.h"
#include "../api.h"
#include "../music.h"
#include "../image_cache.h"
#include <stdlib.h>
#define N(x) scenes_location_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (MAX(STR_AT_EVENT_LOCATION_LEN, STR_AT_TRAIN_STATION_LEN, STR_AT_PLAZA_LEN, STR_AT_MALL_LEN, STR_AT_BEACH_LEN, STR_AT_ARCADE_LEN, STR_AT_CATCAFE_LEN) + STR_CHECK_INBOXES_LEN + STR_BACK_ALLEY_LEN + STR_SETTINGS_LEN + STR_EXIT_LEN)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_location;
	C2D_Text g_subtitle;
	C2D_Text g_entries[4];
	C2D_Text artist;
	C2D_Image background;
	C2D_SpriteSheet spr;
	int cursor;
	float width;
	float artist_width;
	bool event_location;
	bool view_bg_only;
	int location_id;
} N(DataStruct);

LanguageString* N(locations)[NUM_LOCATIONS] = {
	&str_at_train_station,
	&str_at_plaza,
	&str_at_mall,
	&str_at_beach,
	&str_at_arcade,
	&str_at_catcafe,
};

const char* N(music)[NUM_LOCATIONS] = {
	"train_station",
	"plaza",
	"mall",
	"beach",
	"arcade",
	"cat_cafe",
};

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	memset(sc->d, 0, sizeof(N(DataStruct)));
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + 50);
	_data->cursor = 0;
	_data->event_location = location.id == -2;
	_data->view_bg_only = false;
	_data->location_id = (int)sc->data;
	if (_data->event_location) {
		TextLangParse(&_data->g_location, _data->g_staticBuf, str_at_event_location);
		C2D_TextParse(&_data->g_subtitle, _data->g_staticBuf, location.name);
	} else {
		TextLangParse(&_data->g_location, _data->g_staticBuf, *N(locations)[location.id >= 0 && location.id < NUM_LOCATIONS ? location.id : 0]);
	}
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_check_inboxes);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_back_alley);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_exit);
	
	if (*location.artist_name) {
		char string[150];
		snprintf(string, 150, _s(str_artist_copyright), location.artist_name);
		C2D_TextFontParse(&_data->artist, _font(str_artist_copyright), _data->g_staticBuf, string);
		get_text_dimensions(&_data->artist, 0.4, 0.4, &_data->artist_width, 0);
		_data->artist_width += 4.0;
	} else {
		_data->artist_width = 0;
	}
	
	get_text_dimensions(&_data->g_location, 1, 1, &_data->width, 0);
	for (int i = 0; i < 4; i++) {
		float width;
		get_text_dimensions(&_data->g_entries[i], 1, 1, &width, 0);
		width += 20.;
		if (width > _data->width) {
			_data->width = width;
		}
	}
	if (!get_current_location_image(&_data->background)) {
		if (_data->event_location) {
			_data->spr = C2D_SpriteSheetLoad("romfs:/gfx/event_location.t3x");
			playMusic("home");
		} else if (location.id >= 0 && location.id < NUM_LOCATIONS) {
			_data->spr = C2D_SpriteSheetLoad("romfs:/gfx/locations.t3x");
			playMusic(N(music)[location.id]);
		}
	}
}

void N(render)(Scene* sc) {
	if (!_data) return;
	if (_data->spr) {
		if (_data->event_location) {
			C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, 0);
			C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
		} else if (C2D_SpriteSheetCount(_data->spr) > location.id) {
			C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, location.id);
			C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
		}
	} else if (_data->background.tex) {
		C2D_DrawImageAt(_data->background, 0, 0, 0, NULL, 1, 1);
	}
	
	if (_data->view_bg_only) return;
	
	u32 bgclr = C2D_Color32(0, 0, 0, 0x50);
	u32 clr = C2D_Color32(0xff, 0xff, 0xff, 0xff);
	
	if (_data->artist_width != 0) {
		C2D_DrawRectSolid(SCREEN_TOP_WIDTH - _data->artist_width, SCREEN_TOP_HEIGHT - 12, 0, _data->artist_width, 12, bgclr);
		C2D_DrawText(&_data->artist, C2D_AlignRight | C2D_WithColor, SCREEN_TOP_WIDTH - 2, SCREEN_TOP_HEIGHT - 12, 0, 0.4, 0.4, clr);
	}
	
	C2D_DrawRectSolid(8, 8, 0, _data->width + 4, 10 + (_data->event_location ? 6 : 5)*25, bgclr);
	C2D_DrawText(&_data->g_location, C2D_AlignLeft | C2D_WithColor, 10, 10, 0, 1, 1, clr);
	if (_data->event_location) {
		C2D_DrawText(&_data->g_subtitle, C2D_AlignLeft | C2D_WithColor, 10, 10 + 25, 0, 1, 1, clr);
	}
	for (int i = 0; i < 4; i++) {
		C2D_DrawText(&_data->g_entries[i], C2D_AlignLeft | C2D_WithColor, 30, 10 + (i+(_data->event_location ? 2 : 1))*25, 0, 1, 1, clr);
	}
	int x = 10;
	int y = 10 + (_data->cursor + (_data->event_location ? 2 : 1))*25 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 0);
}

void N(exit)(Scene* sc) {
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

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	if (_data) {
		_data->view_bg_only = (kHeld & KEY_L) || (kHeld & KEY_R);
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->cursor < 0) _data->cursor = 3;
		if (_data->cursor > 3) _data->cursor = 0;
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					triggerDownloadInboxes();
				}));
				return scene_push;
			}
			if (_data->cursor == 1) {
				sc->next_scene = getBackAlleyScene();
				return scene_push;
			}
			if (_data->cursor == 2) {
				sc->next_scene = getSettingsScene();
				return scene_push;
			}
			if (_data->cursor == 3) return scene_stop;
		}
		if (location.id == -1) {
			sc->next_scene = getHomeScene();
			return scene_switch;
		}
		if (location.id != _data->location_id) {
			sc->next_scene = getLocationScene(location.id);
			return scene_switch;
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getLocationScene(int location_id) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->is_popup = false;
	scene->need_free = true;
	scene->data = (u32)location_id;
	return scene;
}
