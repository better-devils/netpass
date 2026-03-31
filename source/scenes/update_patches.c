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

#include "update_patches.h"
#include "../config.h"
#include <stdlib.h>
#define _data ((DataStruct*)sc->d)
#define TEXT_BUF_LEN (STR_UPDATE_PATCHES_LEN + STR_UPDATE_PATCHES_DESC_LEN + STR_UPDATE_PATCHES_ERROR_DESC_LEN + STR_UPDATE_PATCHES_POWEROFF_DESC_LEN + STR_UPDATE_PATCHES_POWEROFF_CLEAR_DESC_LEN + STR_BACK_LEN + STR_SKIP_LEN + STR_INSTALL_LEN + STR_REMOVE_LEN + STR_A_OK_LEN)

enum State {Question, Poweroff, Error, Pop, Poweroff_Clear};

typedef struct {
	int state;
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_description;
	C2D_Text g_description_error;
	C2D_Text g_description_poweroff;
	C2D_Text g_description_poweroff_clear;
	C2D_Text g_install;
	C2D_Text g_skip;
	C2D_Text g_back;
	C2D_Text g_remove;
	C2D_Text g_a_ok;
	int cursor;
} DataStruct;

static DataStruct* __data__;

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	__data__ = sc->d;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	_data->state = Question;
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_update_patches);
	TextLangParse(&_data->g_description, _data->g_staticBuf, str_update_patches_desc);
	TextLangParse(&_data->g_description_error, _data->g_staticBuf, str_update_patches_error_desc);
	TextLangParse(&_data->g_description_poweroff, _data->g_staticBuf, str_update_patches_poweroff_desc);
	TextLangParse(&_data->g_description_poweroff_clear, _data->g_staticBuf, str_update_patches_poweroff_clear_desc);
	TextLangParse(&_data->g_install, _data->g_staticBuf, str_install);
	TextLangParse(&_data->g_skip, _data->g_staticBuf, str_skip);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);
	TextLangParse(&_data->g_remove, _data->g_staticBuf, str_remove);
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, str_a_ok);
	_data->cursor = 0;
}

static void render(Scene* sc) {
	if (!_data) return;
	renderPlainText(&_data->g_title, 10, 10, 1, 0);
	if (_data->state == Question) {
		renderPlainTextFlags(&_data->g_description, C2D_WordWrap, 30, 38, 0.5, 0, 350.f);
		if (sc->next_scene) {
			// only two menu entries
			renderPlainText(&_data->g_install, 30, 172, 0.75, 0);
			renderPlainText(&_data->g_skip, 30, 191, 0.75, 0);
		} else {
			// all three menu entries
			renderPlainText(&_data->g_install, 30, 172, 0.75, 0);
			renderPlainText(&_data->g_remove, 30, 191, 0.75, 0);
			renderPlainText(&_data->g_back, 30, 210, 0.75, 0);
		}
		int x = 13;
		int y = 172 + 1 + _data->cursor*19 + 5;
		u32 clr = C2D_Color32(0, 0, 0, 0xff);
		C2D_DrawTriangle(x, y, clr, x, y + 13, clr, x + 11, y + 7, clr, 0);
	} else if (_data->state == Poweroff) {
		renderPlainTextFlags(&_data->g_description_poweroff, C2D_WordWrap, 30, 38, 0.5, 0, 350.f);
		renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, 370, 200, 1, 0);
	} else if (_data->state == Poweroff_Clear) {
		renderPlainTextFlags(&_data->g_description_poweroff_clear, C2D_WordWrap, 30, 38, 0.5, 0, 350.f);
		renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, 370, 200, 1, 0);
	} else {
		renderPlainTextFlags(&_data->g_description_error, C2D_WordWrap, 30, 38, 0.5, 0, 350.f);
		renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, 370, 200, 1, 0);
	}
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
		if (_data->state == Pop) {
			return scene_pop;
		}
		if (_data->state == Question) {
			int num_entries = sc->next_scene ? 2 : 3;
			_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
			if (_data->cursor < 0) _data->cursor = num_entries - 1;
			if (_data->cursor > num_entries - 1) _data->cursor = 0;
		}
		if ((_data->state == Question && (kDown & KEY_B)) || (_data->state == Error && (kDown & KEY_A))) {
			if (sc->next_scene) return scene_switch;
			return scene_pop;
		}
		if (_data->state == Question && (kDown & KEY_A)) {
			if (_data->cursor == 0) {
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					if (writePatches()) {
						__data__->state = Poweroff;
					} else {
						__data__->state = Error;
					}
				}));
				return scene_push;
			}
			if (_data->cursor == 1) {
				if (sc->next_scene) {
					return scene_switch;
				}
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					if (clearPatches()) {
						__data__->state = Poweroff_Clear;
					} else {
						__data__->state = Pop;
					}
				}));
				return scene_push;
			}
			return scene_pop;
		}
		if ((_data->state == Poweroff || _data->state == Poweroff_Clear) && (kDown & KEY_A)) {
			clearBossCacheAndReboot();
			return scene_continue;
		}
	}
	return scene_continue;
}

Scene* getUpdatePatchesScene(Scene* next_scene) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	scene->next_scene = next_scene;
	return scene;
}
