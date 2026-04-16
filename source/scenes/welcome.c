/**
 * NetPass
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

#include "welcome.h"
#include "../config.h"
#include "info.h"
#define _data ((DataStruct*)sc->d)
#define TEXT_BUF_LEN (STR_WELCOME_LEN + STR_WELCOME_MESSAGE_LEN + STR_VIEW_RULES_LEN + STR_VIEW_PRIVACY_LEN + STR_CONTINUE_LEN + STR_SCAM_WARNING_LEN)

#define NUM_ENTRIES 3

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_subtitle;
	C2D_Text g_scam_warning;
	C2D_Text g_entries[NUM_ENTRIES];
	int cursor;
} DataStruct;

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_welcome);
	TextLangParse(&_data->g_subtitle, _data->g_staticBuf, str_welcome_message);
	TextLangParse(&_data->g_scam_warning, _data->g_staticBuf, str_scam_warning);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_view_rules);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_view_privacy);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_continue);
	_data->cursor = 0;
}

static void render(Scene* sc) {
	if (!_data) return;
	renderPlainText(&_data->g_title, 10, 10, 1, 0);
	renderPlainTextFlags(&_data->g_subtitle, C2D_WordWrap, 11, 40, 0.5, 0, 369.);
	renderPlainTextFlags(&_data->g_scam_warning, C2D_WordWrap, 11, 69, 0.5, 0, 369.);
	for (int i = 0; i < NUM_ENTRIES; i++) {
		renderPlainText(&_data->g_entries[i], 30, 114 + (i * 25), 1, 0);
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	int x = 10;
	int y = _data->cursor*25 + 114 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 0);
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
		if (_data->cursor < 0) _data->cursor = NUM_ENTRIES - 1;
		if (_data->cursor > NUM_ENTRIES - 1) _data->cursor = 0;
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				open_url(RULES_URL);
			}
			if (_data->cursor == 1) {
				open_url(PRIVACY_URL);
			}
			if (_data->cursor == 2) {
				config.welcome_version = _WELCOME_VERSION_;
				configWrite();
				return scene_switch;
			}
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getWelcomeScene(Scene* next_scene) {
	Scene* popup = getInfoScene(str_backup_nid_pwd);
	if (!popup) return NULL;
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	popup->pop_scene = next_scene;
	scene->next_scene = popup;
	return scene;
}
