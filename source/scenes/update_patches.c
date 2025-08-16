/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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
#define N(x) scenes_update_patches_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
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
} N(DataStruct);

N(DataStruct)* __data__;

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
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

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, 10, 0, 1, 1);
	if (_data->state == Question) {
		C2D_DrawText(&_data->g_description, C2D_AlignLeft | C2D_WordWrap, 30, 38, 0, 0.5, 0.5, 350.);
		if (sc->next_scene) {
			// only two menu entries
			C2D_DrawText(&_data->g_install, C2D_AlignLeft, 30, 172, 0, 0.75, 0.75);
			C2D_DrawText(&_data->g_skip, C2D_AlignLeft, 30, 191, 0, 0.75, 0.75);
		} else {
			// all three menu entries
			C2D_DrawText(&_data->g_install, C2D_AlignLeft, 30, 172, 0, 0.75, 0.75);
			C2D_DrawText(&_data->g_remove, C2D_AlignLeft, 30, 191, 0, 0.75, 0.75);
			C2D_DrawText(&_data->g_back, C2D_AlignLeft, 30, 210, 0, 0.75, 0.75);
		}
		int x = 13;
		int y = 172 + 1 + _data->cursor*19 + 5;
		u32 clr = C2D_Color32(0, 0, 0, 0xff);
		C2D_DrawTriangle(x, y, clr, x, y + 13, clr, x + 11, y + 7, clr, 0);
	} else if (_data->state == Poweroff) {
		C2D_DrawText(&_data->g_description_poweroff, C2D_AlignLeft | C2D_WordWrap, 30, 38, 0, 0.5, 0.5, 350.);
		C2D_DrawText(&_data->g_a_ok, C2D_AlignRight, 370, 200, 0, 1, 1);
	} else if (_data->state == Poweroff_Clear) {
		C2D_DrawText(&_data->g_description_poweroff_clear, C2D_AlignLeft | C2D_WordWrap, 30, 38, 0, 0.5, 0.5, 350.);
		C2D_DrawText(&_data->g_a_ok, C2D_AlignRight, 370, 200, 0, 1, 1);
	} else {
		C2D_DrawText(&_data->g_description_error, C2D_AlignLeft | C2D_WordWrap, 30, 38, 0, 0.5, 0.5, 350.);
		C2D_DrawText(&_data->g_a_ok, C2D_AlignRight, 370, 200, 0, 1, 1);
	}
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
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->next_scene = next_scene;
	scene->is_popup = false;
	scene->need_free = true;
	return scene;
}
