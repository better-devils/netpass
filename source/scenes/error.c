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

#include "error.h"
#include "../ctr_results.h"
#include <curl/curl.h>

#define _data ((DataStruct*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_origin;
	C2D_Text g_title;
	C2D_Text g_subtext;
	C2D_Text g_a_ok;
	ErrorData* err;
} DataStruct;

#define WIDTH_SCR 400
#define HEIGHT_SCR 240
#define MARGIN 20
#define WIDTH (WIDTH_SCR - 2*MARGIN)
#define HEIGHT (HEIGHT_SCR - 2*MARGIN)

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(1500);
	_data->err = (ErrorData*)sc->data;
	char str[200] = {0};
	char subtext[1000] = {0};
	char origintext[200] = {0};
	C2D_Font str_font;
	C2D_Font subtext_font = 0;
	
	snprintf(origintext, sizeof(origintext), "Origin: %s(%s:%d)", _data->err->func, _data->err->file, _data->err->line);
	do {
		if (ERROR_IS_HTTP(_data->err->error)) {
			// http status code
			int status_code = -_data->err->error;
			snprintf(str, sizeof(str), _s(str_httpstatus_error), status_code);
			str_font = _font(str_httpstatus_error);
			break;
		}
		if (ERROR_IS_CURL(_data->err->error)) {
			// libcurl error code
			int errcode = -_data->err->error;
			const char* errmsg = curl_easy_strerror(errcode);
			snprintf(str, sizeof(str), _s(str_libcurl_error), errcode, errmsg);
			str_font = _font(str_libcurl_error);
			if (errcode == 60) {
				strncpy(subtext, _s(str_libcurl_date_and_time), sizeof(subtext) - 1);
				subtext_font = _font(str_libcurl_date_and_time);
			}
			break;
		}
		if (_data->err->error == ERROR_ERRNO || _data->err->std_errno) {
			// errno error
			strerror_r(_data->err->std_errno, str, sizeof(str));
			str_font = 0;
			break;
		}
		
		// 3ds error code
		snprintf(str, sizeof(str), _s(str_3ds_error), (u32)_data->err->error);
		str_font = _font(str_3ds_error);
		char module[200] = {0};
		char description[200] = {0};
		get_module_formatted(module, sizeof(module), _data->err->error);
		get_description_formatted(description, sizeof(description), _data->err->error);
		snprintf(
			subtext, sizeof(subtext), "Description: %s\nLevel: %s (%d)\nModule: %s",
			description, get_level_string(_data->err->error), (int)CTR_RESULT_GET_LEVEL(_data->err->error), module
		);
		break;
	} while(1);
	C2D_TextParse(&_data->g_origin, _data->g_staticBuf, origintext);
	C2D_TextFontParse(&_data->g_title, str_font, _data->g_staticBuf, str);
	C2D_TextFontParse(&_data->g_subtext, subtext_font, _data->g_staticBuf, subtext);
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, str_a_ok);
}

static void render(Scene* sc) {
	if (!_data) return;
	C2D_DrawRectSolid(MARGIN, MARGIN, 0, WIDTH, HEIGHT, C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF));
	renderText(&_data->g_origin, MARGIN + 5, MARGIN + 5, 0.5, 0);
	renderPlainTextFlags(&_data->g_title, C2D_WordWrap, MARGIN + 5, MARGIN + 5 + 25, 0.5, 0, (WIDTH - 2*MARGIN - 10) * 1.f);
	renderPlainTextFlags(&_data->g_subtext, C2D_WordWrap, MARGIN + 5, MARGIN + 5 + 50, 0.5, 0, (WIDTH - 2*MARGIN - 10) * 1.f);
	renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, MARGIN + WIDTH - 5, MARGIN + HEIGHT - 30, 1, 0);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		free((int*)sc->data);
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & (KEY_A | KEY_B)) {
		return scene_pop;
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getErrorScene(ErrorData* error) {
	Scene* scene = createScene(sizeof(ErrorData));
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	memcpy((void*)scene->data, error, sizeof(ErrorData));
	scene->is_popup = true;
	return scene;
}
