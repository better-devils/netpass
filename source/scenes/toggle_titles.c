/**
 * NetPass
 * Copyright (C) 2024 SunOfLife1
 * Copyright (C) 2025 Sorunome
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

#include "toggle_titles.h"
#include "../config.h"
#include <stdlib.h>
#define TEXT_BUF_LEN (STR_TOGGLE_TITLES_LEN + STR_TOGGLE_TITLES_MESSAGE_LEN + STR_BACK_LEN + STR_TOGGLE_TITLES_ON_LEN + STR_TOGGLE_TITLES_OFF_LEN)

#define _data ((DataStruct*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_header;
	C2D_Text g_subtext;
	C2D_Text g_on_off[2];
	C2D_Text g_game_titles[24];
	u32 title_ids[24];
	C2D_Text g_back;
	int cursor;
	int number_games;
} DataStruct;

static bool init_gamelist(Scene* sc) {
	NetpassTitleData* title_data = getTitleData();
	for (int i = 0; i < title_data->num_titles; i++) {
		_data->title_ids[i] = title_data->titles[i].title_id;
		C2D_TextParse(&_data->g_game_titles[i], _data->g_staticBuf, title_data->titles[i].name);
	}
	_data->number_games = title_data->num_titles;
	return true;
}

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + 12 * 24);
	if (!init_gamelist(sc)) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		sc->d = 0;
		return;
	}
	
	_data->cursor = 0;
	TextLangParse(&_data->g_header, _data->g_staticBuf, str_toggle_titles);
	TextLangParse(&_data->g_subtext, _data->g_staticBuf, str_toggle_titles_message);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);
	TextLangParse(&_data->g_on_off[0], _data->g_staticBuf, str_toggle_titles_off);
	TextLangParse(&_data->g_on_off[1], _data->g_staticBuf, str_toggle_titles_on);
}

static void render(Scene* sc) {
	if (!_data) return;
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	u32 onClr = C2D_Color32(10, 200, 10, 0xff);
	u32 offClr = C2D_Color32(200, 10, 10, 0xff);
	C2D_DrawText(&_data->g_header, C2D_AlignLeft | C2D_WithColor, 10, 10, 0, 1, 1, clr);
	C2D_DrawText(&_data->g_subtext, C2D_AlignLeft | C2D_WithColor, 11, 40, 0, 0.5, 0.5, clr);
	int i = 0;
	for (; i < _data->number_games; i++) {
		bool isIgnored = isTitleIgnored(_data->title_ids[i]);
		u32 correctClr = isIgnored ? offClr : onClr;
		C2D_DrawText(&_data->g_on_off[!isIgnored], C2D_AlignLeft | C2D_WithColor, 30, 60 + (i * 14), 0, 0.5, 0.5, correctClr);
		C2D_DrawText(&_data->g_game_titles[i], C2D_AlignLeft | C2D_WithColor, 70, 60 + (i * 14), 0, 0.5, 0.5, correctClr);
	}
	C2D_DrawText(&_data->g_back, C2D_AlignLeft | C2D_WithColor, 30, 60 + (i*14), 0, 0.5, 0.5, clr);
	
	int x = 22;
	int y = _data->cursor*14 + 60 + 3;
	C2D_DrawTriangle(x, y, clr, x, y + 10, clr, x + 8, y + 5, clr, 0);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (_data) {
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->cursor < 0) _data->cursor = _data->number_games;
		if (_data->cursor > _data->number_games) _data->cursor = 0;
		if (kDown & KEY_A) {
			// "Back" is selected, exit this scene
			if (_data->cursor == _data->number_games) {
				configWrite();
				return scene_pop;
			}

			// Add title to ignore list / Remove title from ignore list
			u32 title_id = _data->title_ids[_data->cursor];
			if (isTitleIgnored(title_id)) {
				removeIgnoredTitle(title_id);
			} else {
				addIgnoredTitle(title_id);
			}
			return scene_continue;
		}
		if (kDown & KEY_B) {
			configWrite();
			return scene_pop;
		}
	}
	if (kDown & KEY_B) {
		configWrite();
		return scene_pop;
	}
	if (kDown & KEY_START) {
		configWrite();
		return scene_stop;
	}
	return scene_continue;
}

Scene* getToggleTitlesScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
