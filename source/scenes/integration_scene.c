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

#include "integration_scene.h"
#include "../integration.h"
#define N(x) scenes_integration_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_INTEGRATIONS_LEN + STR_INTEGRATIONS_MESSAGE_LEN + STR_BACK_LEN + STR_TOGGLE_TITLES_OFF_LEN + STR_TOGGLE_TITLES_ON_LEN)

typedef struct {
	int cursor;
	IntegrationList* list;
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_subtitle;
	C2D_Text g_back;
	C2D_Text g_on_off[2];
	C2D_Text* integration_names;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->cursor = 0;
	_data->list = get_integration_list();
	if (!_data->list) {
		free(_data);
		sc->d = NULL;
		return;
	}
	_data->integration_names = malloc(sizeof(C2D_Text) * _data->list->header.count);
	if (!_data->integration_names) {
		free(_data->list);
		free(_data);
		sc->d = NULL;
		return;
	}
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + (_data->list->header.count*24));
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_integrations);
	TextLangParse(&_data->g_subtitle, _data->g_staticBuf, str_integrations_message);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);
	TextLangParse(&_data->g_on_off[0], _data->g_staticBuf, str_toggle_titles_off);
	TextLangParse(&_data->g_on_off[1], _data->g_staticBuf, str_toggle_titles_on);
	for (int i = 0; i < _data->list->header.count; i++) {
		C2D_TextParse(&_data->integration_names[i], _data->g_staticBuf, _data->list->entries[i].name);
	}
}

void N(render)(Scene* sc) {
	if (!_data) return;
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	u32 onClr = C2D_Color32(10, 200, 10, 0xff);
	u32 offClr = C2D_Color32(200, 10, 10, 0xff);
	C2D_DrawText(&_data->g_title, C2D_AlignLeft | C2D_WithColor, 10, 10, 0, 1, 1, clr);
	C2D_DrawText(&_data->g_subtitle, C2D_AlignLeft | C2D_WithColor | C2D_WordWrap, 11, 40, 0, 0.5, 0.5, clr, 369.);
	int i = 0;
	for (;i < _data->list->header.count; i++) {
		bool is_on = _data->list->entries[i].enabled;
		u32 correctClr = is_on ? onClr : offClr;
		C2D_DrawText(&_data->g_on_off[is_on], C2D_AlignLeft | C2D_WithColor, 30, 74 + (i * 14), 0, 0.5, 0.5, correctClr);
		C2D_DrawText(&_data->integration_names[i], C2D_AlignLeft | C2D_WithColor, 70, 74 + (i * 14), 0, 0.5, 0.5, correctClr);
	}
	C2D_DrawText(&_data->g_back, C2D_AlignLeft | C2D_WithColor, 30, 74 + (i*14), 0, 0.5, 0.5, clr);
	
	int x = 22;
	int y = _data->cursor*14 + 74 + 3;
	C2D_DrawTriangle(x, y, clr, x, y + 10, clr, x + 8, y + 5, clr, 0);
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
		if (_data->cursor < 0) _data->cursor = _data->list->header.count;
		if (_data->cursor > _data->list->header.count) _data->cursor = 0;
		if (kDown & KEY_A) {
			// if back is selected
			if (_data->cursor == _data->list->header.count) {
				return scene_pop;
			}
			Result res = toggle_integration(_data->list->entries[_data->cursor].id);
			_e(res);
			return scene_continue;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getIntegrationScene(void) {
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
