/**
 * NetPass
 * Copyright (C) 2026 Sorunome
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

#include "set_time.h"
#include "loading.h"
#include "prompt.h"
#include "switch.h"
#define _data ((DataStruct*)sc->d)
#define _initdata ((InitData*)sc->data)
#define TEXT_BUF_LEN (STR_A_OK_LEN + STR_SET_TIME_LEN)

extern char* filename_3dsx;

typedef struct {
	time_t date;
} InitData;

static time_t offset;

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_set_time;
	C2D_Text g_a_ok;
	time_t offset;
	int cursor;
} DataStruct;

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	memset(sc->d, 0, sizeof(DataStruct));
	time_t now = time(NULL);
	const time_t day = 60*60*24;
	int sign = _initdata->date < now ? -1 : 1;
	time_t num_days_offset = llabs(now - _initdata->date) / day;
	time_t remainder = llabs(now - _initdata->date) % day;
	if (remainder > day/2) num_days_offset++;
	_data->offset = num_days_offset * day * sign;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	TextLangParse(&_data->g_set_time, _data->g_staticBuf, str_set_time);
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, str_a_ok);
}

static void render(Scene* sc) {
	if (!_data) return;
	renderPlainTextFlags(&_data->g_set_time, C2D_WordWrap, 10, 10, 0.5, 0, SCREEN_TOP_WIDTH*1.f);
	
	char timestr[20];
	struct tm tm;
	time_t now = time(NULL);
	time_t ts = now + _data->offset;
	if (localtime_r(&ts, &tm)) {
		n_strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tm);
	} else {
		memset(&tm, 0, sizeof(tm));
	}
	
	C2D_TextBuf tmpbuf = C2D_TextBufNew(20);
	C2D_Text curtime;
	C2D_TextParse(&curtime, tmpbuf, timestr);
	renderPlainTextFlags(&curtime, C2D_AlignLeft, 57, (int)(SCREEN_TOP_HEIGHT/2) - 12, 1.0, 0);
	C2D_TextBufDelete(tmpbuf);
	
	const int x_map[] = {85, 146, 192, 236, 278, 322};
	int x = x_map[_data->cursor];
	int y = now % 2 ? 5 : 0;
	C2D_DrawTriangle(x, (int)(SCREEN_TOP_HEIGHT/2) - 10 - y, clr_black, x + 12, (int)(SCREEN_TOP_HEIGHT/2) - 10 - y, clr_black, x + 6, (int)(SCREEN_TOP_HEIGHT/2) - 19 - y, clr_black, 0);
	C2D_DrawTriangle(x, (int)(SCREEN_TOP_HEIGHT/2) + 17 + y, clr_black, x + 12, (int)(SCREEN_TOP_HEIGHT/2) + 17 + y, clr_black, x + 6, (int)(SCREEN_TOP_HEIGHT/2) + 26 + y, clr_black, 0);
	
	renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, SCREEN_TOP_WIDTH - 5, SCREEN_TOP_HEIGHT - 30, 1, 0);
}

static void exit_scene(Scene* sc) {
	if (!_data) return;
	if (_data->g_staticBuf) C2D_TextBufDelete(_data->g_staticBuf);
	free(_data);
	sc->d = 0;
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (_data) {
		if (kDown & KEY_A && _data->cursor == 5) {
			offset = _data->offset;
			Scene* new_scene = getPromptScene(str_time_correct, getSwitchScene(lambda(Scene*, (void) {
				// set the actual time here
				s64 msY2K = time(NULL) + offset - 946684800;
				msY2K *= 1000;
				
				Result res = _e(ptmSysmInit());
				if (R_FAILED(res)) return getInfoScene(str_time_failed);
				res = _e(ptmSetsInit());
				if (R_FAILED(res)) return getInfoScene(str_time_failed);
				res = _e(PTMSYSM_SetUserTime(msY2K));
				if (R_FAILED(res)) return getInfoScene(str_time_failed);
				res = _e(PTMSETS_SetSystemTime(msY2K));
				if (R_FAILED(res)) return getInfoScene(str_time_failed);
				
				Scene* new_scene = getInfoScene(str_time_success);
				new_scene->pop_scene = getRestartScene();
				return new_scene;
			})));
			sc->next_scene = new_scene;
			return scene_push;
		}
		_data->cursor += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT || kDown & KEY_A) && 1) - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT || kDown & KEY_B) && 1);
		if (_data->cursor <	0) _data->cursor = 0;
		if (_data->cursor > 5) _data->cursor = 5;
		int sign = ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1) - ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1);
		if (sign != 0) {
			struct tm tm;
			time_t now = time(NULL);
			time_t ts = now + _data->offset;
			if (localtime_r(&ts, &tm)) {
				switch (_data->cursor) {
					case 0: // years
						tm.tm_year += sign;
						break;
					case 1: // months
						tm.tm_mon += sign;
						break;
					case 2: // days
						tm.tm_mday += sign;
						break;
					case 3: // hours
						tm.tm_hour += sign;
						break;
					case 4: // minutes
						tm.tm_min += sign;
						break;
					case 5: // seconds
						tm.tm_sec += sign;
						break;
				}
				_data->offset += mktime(&tm) - ts;
			}
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getSetTimeScene(time_t date) {
	Scene* scene = createScene(sizeof(InitData));
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	((InitData*)scene->data)->date = date;
	return scene;
}
