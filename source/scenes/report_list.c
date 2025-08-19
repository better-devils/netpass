/**
 * NetPass
 * Copyright (C) 2024-2025 Sorunome
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

#include "report_list.h"
#include "../report.h"
#include <stdlib.h>
#include <malloc.h>
#define N(x) scenes_report_list_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	ReportList* list;
	C2D_Text* g_entries;
	int cursor;
	int offset;
} N(DataStruct);

char* N(send_msg);
u32 N(send_transfer_id);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;

	_data->list = loadReportList();

	if (!_data->list) {
		_e(-1);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_entries = malloc(sizeof(C2D_Text) * _data->list->header.cur_size);
	if (!_data->g_entries) {
		free(_data->list);
		free(_data);
		sc->d = NULL;
		return;
	}
	_data->cursor = 0;
	_data->offset = 0;
	_data->g_staticBuf = C2D_TextBufNew(40 * _data->list->header.cur_size);

	for (int i = 0; i < _data->list->header.cur_size; i++) {
		ReportListEntry* entry = &_data->list->entries[i];
		u8 mii_name[MII_UTF8_NAME_LEN];
		get_mii_name(mii_name, &entry->mii);
		char timestr[60];
		struct tm tm;
		cecTimeToTm(&entry->received, &tm);
		n_strftime(timestr, sizeof(timestr), _s(str_date_time), &tm);
		char render_entry[sizeof(timestr) + 2 + MII_UTF8_NAME_LEN];
		snprintf(render_entry, sizeof(timestr) + 2 + MII_UTF8_NAME_LEN, "%s  %s", mii_name, timestr);
		C2D_TextFontParse(&_data->g_entries[i], getFontIndex(entry->mii.mii_options.char_set), _data->g_staticBuf, render_entry);
	}
}

void N(render)(Scene* sc) {
	if (!_data) {
		return;
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	for (int i = 0; i < _data->list->header.cur_size; i++) {
		int ix = _data->list->header.cur_size - i - 1;
		int x = 35 + i*14 - _data->offset;
		if (x > -14 && x < 240) {
			C2D_DrawText(&_data->g_entries[ix], C2D_AlignLeft | C2D_WithColor, 30, x, 0, 0.5, 0.5, clr);
		}
	}
	int x = 22;
	int y = 35 + _data->cursor*14 + 3 - _data->offset;
	C2D_DrawTriangle(x, y, clr, x, y +10, clr, x + 8, y + 5, clr, 0);
}

void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data->g_entries);
		free(_data->list);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (!_data) return scene_pop;
	
	_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
	_data->cursor += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT) && 1)*10 - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT) && 1)*10;
	if (_data->cursor < 0) _data->cursor = (_data->list->header.cur_size-1);
	if (_data->cursor > (_data->list->header.cur_size-1)) _data->cursor = 0;
	while(_data->cursor*14 - _data->offset < 2) _data->offset--;
	while(_data->cursor*14 - _data->offset > 180) _data->offset++;
	if (kDown & KEY_A) {
		int selected_i = _data->list->header.cur_size - _data->cursor - 1;
		ReportListEntry* entry = &_data->list->entries[selected_i];
		sc->next_scene = getReportEntryScene(entry);
		return scene_push;
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getReportListScene(void) {
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
