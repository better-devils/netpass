/**
 * NetPass
 * Copyright (C) 2024-2026 Sorunome
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

#include "report_list.h"
#include "../report.h"
#include "info.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#define _data ((DataStruct*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	FILE* file;
	u32 num_entries;
	C2D_Text* g_entries;
	int cursor;
	int offset;
} DataStruct;

static void exit_scene(Scene* sc);

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	memset(_data, 0, sizeof(DataStruct));
	if (R_FAILED(_e(loadReportList(&_data->file)))) return exit_scene(sc);

	if (fread(&_data->num_entries, sizeof(&_data->num_entries), 1, _data->file) != 1) {
		_e_errno();
		return exit_scene(sc);
	}

	_data->g_entries = malloc(sizeof(C2D_Text) * _data->num_entries);
	if (!_data->g_entries) {
		_e_errno();
		return exit_scene(sc);
	}
	
	_data->g_staticBuf = C2D_TextBufNew(40 * _data->num_entries);
	if (!_data->g_staticBuf) {
		_e(ERROR_OUT_OF_MEMORY);
		return exit_scene(sc);
	}
	
	ReportListEntry entry;
	
	for (int i = 0; i < _data->num_entries; i++) {
		if (fread(&entry, sizeof(entry), 1, _data->file) != 1) {
			_e_errno();
			return exit_scene(sc);
		}
		char timestr[60];
		struct tm tm;
		if (localtime_r(&entry.time, &tm)) {
			n_strftime(timestr, sizeof(timestr), _s(str_date_time), &tm);
		} else {
			timestr[0] = 0;
		}
		
		char render_entry[sizeof(timestr) + 2 + 25];

		const char prefix[3] = "CB?";
		snprintf(render_entry, sizeof(timestr) + 2 + 25, "(%c) %s  %s", prefix[MIN(entry.type, sizeof(prefix)-1)], entry.name, timestr);
		C2D_TextParse(&_data->g_entries[i], _data->g_staticBuf, render_entry);
	}
}

static void render(Scene* sc) {
	if (!_data) {
		return;
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	for (int i = 0; i < _data->num_entries; i++) {
		int x = 35 + i*14 - _data->offset;
		if (x > -14 && x < 240) {
			renderPlainText(&_data->g_entries[i], 30, x, 0.5, clr);
		}
	}
	int x = 22;
	int y = 35 + _data->cursor*14 + 3 - _data->offset;
	C2D_DrawTriangle(x, y, clr, x, y +10, clr, x + 8, y + 5, clr, 0);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		if (_data->g_staticBuf) C2D_TextBufDelete(_data->g_staticBuf);
		if (_data->g_entries) free(_data->g_entries);
		if (_data->file) fclose(_data->file);
		free(_data);
		sc->d = 0;
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (!_data) return scene_pop;
	
	if (!_data->num_entries) {
		Scene* info = getInfoScene(str_report_list_empty);
		info->pop_scene = sc->pop_scene;
		sc->pop_scene = info;
		return scene_pop;
	}
	
	_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
	_data->cursor += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT) && 1)*10 - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT) && 1)*10;
	if (_data->cursor < 0) _data->cursor = (_data->num_entries-1);
	if (_data->cursor > (_data->num_entries-1)) _data->cursor = 0;
	while(_data->cursor*14 - _data->offset < 2) _data->offset--;
	while(_data->cursor*14 - _data->offset > 180) _data->offset++;
	if (kDown & KEY_A) {
		ReportListEntry entry;
		fseek(_data->file, 4 + (sizeof(entry) * _data->cursor), SEEK_SET);
		if (fread(&entry, sizeof(entry), 1, _data->file) == 1) {
			logln(INFO, "Selected report of type %d, id=%lld, misc_id=%ld, name=%s", entry.type, entry.id, entry.misc_id, entry.name);
			sc->next_scene = getReportEntryScene(entry.id, entry.misc_id, entry.name, entry.type);
			return scene_push;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getReportListScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
