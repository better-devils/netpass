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

#include "error.h"
#include "../ctr_results.h"
#include <curl/curl.h>

#define N(x) scenes_error_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_subtext;
	C2D_Text g_a_ok;
	bool fatal;
	int err;
} N(DataStruct);

#define WIDTH_SCR 400
#define HEIGHT_SCR 240
#define MARGIN 20
#define WIDTH (WIDTH_SCR - 2*MARGIN)
#define HEIGHT (HEIGHT_SCR - 2*MARGIN)

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(1500);
	int* args = (int*)sc->data;
	int err = _data->err = args[0];
	_data->fatal = args[1];
	char str[200] = {0};
	char subtext[1000] = {0};
	C2D_Font str_font;
	C2D_Font subtext_font = 0;
	do {
		if (err > -600 && err <= -200) {
			// http status code
			int status_code = -err;
			snprintf(str, sizeof(str), _s(str_httpstatus_error), status_code);
			str_font = _font(str_httpstatus_error);
			break;
		}
		if (err > -100 && err < 0) {
			// libcurl error code
			int errcode = -err;
			const char* errmsg = curl_easy_strerror(errcode);
			snprintf(str, sizeof(str), _s(str_libcurl_error), errcode, errmsg);
			str_font = _font(str_libcurl_error);
			if (errcode == 60) {
				strncpy(subtext, _s(str_libcurl_date_and_time), sizeof(subtext) - 1);
				subtext_font = _font(str_libcurl_date_and_time);
			}
			break;
		}
		if (err <= -600) {
			// 3ds error code
			snprintf(str, sizeof(str), _s(str_3ds_error), (u32)err);
			str_font = _font(str_3ds_error);
			char level[100] = {0};
			char summary[200] = {0};
			char module[200] = {0};
			char description[200] = {0};
			get_level_formatted(level, sizeof(level), err);
			get_summary_formatted(summary, sizeof(summary), err);
			get_module_formatted(module, sizeof(module), err);
			get_description_formatted(description, sizeof(description), err);
			snprintf(
				subtext, sizeof(subtext), "Level: %s\nModule: %s\nSummary: %s\nDescription: %s",
				level, module, summary, description
			);
			break;
		}
		// errno error
		strerror_r(err, str, sizeof(str));
		str_font = 0;
		break;
	} while(1);
	C2D_TextFontParse(&_data->g_title, str_font, _data->g_staticBuf, str);
	C2D_TextFontParse(&_data->g_subtext, subtext_font, _data->g_staticBuf, subtext);
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, str_a_ok);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawRectSolid(MARGIN, MARGIN, 0, WIDTH, HEIGHT, C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF));
	C2D_DrawText(&_data->g_title, C2D_AlignLeft | C2D_WordWrap, MARGIN + 5, MARGIN + 5, 0, 0.5, 0.5, (WIDTH - 2*MARGIN - 10) * 1.f);
	C2D_DrawText(&_data->g_subtext, C2D_AlignLeft | C2D_WordWrap, MARGIN + 5, MARGIN + 5 + 25, 0, 0.5, 0.5, (WIDTH - 2*MARGIN - 10) * 1.f);
	C2D_DrawText(&_data->g_a_ok, C2D_AlignRight, MARGIN + WIDTH - 5, MARGIN + HEIGHT - 30, 0, 1, 1);
}

void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		free((int*)sc->data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & (KEY_A | KEY_B)) {
		if (_data->fatal) return scene_stop;
		return scene_pop;
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getErrorScene(int err, bool fatal) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	u32* buf = malloc(8);
	if (!buf) {
		free(scene);
		return NULL;
	}
	buf[0] = err;
	buf[1] = fatal;
	scene->data = (u32)buf;
	scene->is_popup = true;
	scene->need_free = true;
	return scene;
}
