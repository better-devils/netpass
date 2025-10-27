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

#include "misc_settings.h"
#include "about.h"
#include "../curl-handler.h"
#define N(x) scenes_misc_settings_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_SETTINGS_LEN + STR_DOWNLOAD_DATA_LEN + STR_DELETE_DATA_LEN + STR_UPDATE_PATCHES_LEN + STR_VIEW_RULES_LEN + STR_VIEW_PRIVACY_LEN + STR_BACK_LEN + STR_LOGGING_LEN)

#define NUM_ENTRIES 8

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_entries[NUM_ENTRIES];
	C2D_Text g_logs[3];
	float logs_width;
	int cursor;
} N(DataStruct);

static void downloadDataThread(void);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	_data->cursor  = 0;
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_settings_about);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_download_data);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_delete_data);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_update_patches);
	TextLangParse(&_data->g_entries[4], _data->g_staticBuf, str_view_privacy);
	TextLangParse(&_data->g_entries[5], _data->g_staticBuf, str_view_rules);
	TextLangParse(&_data->g_entries[6], _data->g_staticBuf, str_logging);
	TextLangParse(&_data->g_entries[7], _data->g_staticBuf, str_back);
	
	C2D_TextGetDimensions(&_data->g_entries[6], 1.0, 1.0, &_data->logs_width, 0);
	
	TextLangParse(&_data->g_logs[0], _data->g_staticBuf, str_logging_none);
	TextLangParse(&_data->g_logs[1], _data->g_staticBuf, str_logging_screen);
	TextLangParse(&_data->g_logs[2], _data->g_staticBuf, str_logging_file);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < NUM_ENTRIES; i++) {
		C2D_DrawText(&_data->g_entries[i], C2D_AlignLeft, 30, 10 + (i+1)*25, 0, 1, 1);
	}
	
	C2D_DrawText(&_data->g_logs[config.log_output], C2D_AlignLeft, 30 + _data->logs_width + 3, 10 + 7*25, 0, 1, 1);
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	int x = 10;
	int y = 10 + (_data->cursor + 1)*25 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 0);
	u32 blue = C2D_Color32(0x2B, 0xCF, 0xFF, 0xFF);
	u32 pink = C2D_Color32(0xF5, 0xAB, 0xB9, 0xFF);
	u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	C2D_DrawRectSolid(400 - 90, 240 - 60, 0, 90, 12, blue);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 12, 0, 90, 12, pink);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 24, 0, 90, 12, white);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 36, 0, 90, 12, pink);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 48, 0, 90, 12, blue);

	C2D_DrawRectSolid(400 - 180, 240 - 60, 0, 90, 10, C2D_Color32(0xE5, 0x00, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 50, 0, 90, 10, C2D_Color32(0xFF, 0x8D, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 40, 0, 90, 10, C2D_Color32(0xFF, 0xEE, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 30, 0, 90, 10, C2D_Color32(0x02, 0x81, 0x21, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 20, 0, 90, 10, C2D_Color32(0x00, 0x4C, 0xFF, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 10, 0, 90, 10, C2D_Color32(0x77, 0x00, 0x88, 0xFF));
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
		if (_data->cursor < 0) _data->cursor = NUM_ENTRIES - 1;
		if (_data->cursor > NUM_ENTRIES - 1) _data->cursor = 0;
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				// About
				sc->next_scene = getAboutScene();
				return scene_push;
			}
			if (_data->cursor == 1) {
				// download personal data
				sc->next_scene = getLoadingScene(0, downloadDataThread);
				return scene_push;
			}
			if (_data->cursor == 2) {
				// delete personal data
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					char url[50];
					snprintf(url, 50, "%s/data", BASE_URL);
					Result res = httpRequest("DELETE", url, 0, 0, 0, 0, 0);
					if (R_FAILED(res)) {
						logln(ERROR, "deleting all data: %ld", res);
						return;
					}
					logln(INFO, "Successfully sent request to delete all data! This can take up to 15 days.");
				}));
				return scene_push;
			}
			if (_data->cursor == 3) {
				// update patches
				sc->next_scene = getUpdatePatchesScene(NULL);
				return scene_push;
			}
			if (_data->cursor == 4) {
				open_url(PRIVACY_URL);
			}
			if (_data->cursor == 5) {
				open_url(RULES_URL);
			}
			if (_data->cursor == 6) {
				config.log_output = (config.log_output + 1) % 3;
				configWrite();
			}
			if (_data->cursor == 7) return scene_pop;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getMiscSettingsScene(void) {
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

static void downloadDataThread(void) {
	time_t now = time(NULL);

	logln(INFO, "Requesting data export:");
	// 42 characters: https://devapi.netpass.cafe/data/download
#define URL_SIZE 52
	char url[URL_SIZE];
	snprintf(url, URL_SIZE, "%s/data/request", BASE_URL);
	Result res = httpRequest("GET", url, 0, NULL, 0, 0, 0);
	if (R_FAILED(res)) {
		logln(ERROR, "%ld", res);
		return;
	}
	if (res != 202) {
		logln(ERROR, "Export: bad status code %ld != 202", res);
		return;
	}
	logln(INFO, "ok.");
	snprintf(url, URL_SIZE, "%s/data/check", BASE_URL);
	logln(INFO, "Waiting..");
#define MAX_WAIT_NANOS 15ULL*1000000000ULL // 15s
	u64 backoff = 500000000ULL; // 0.5s
	while (true) {
		res = httpRequest("GET", url, 0, NULL, 0, 0, 0);
		if (res == 200) {
			logln(INFO, "Ready.");
			break;
		} else if (res == 204) {
			svcSleepThread(backoff);
			backoff += backoff >> 2;
			if (backoff > MAX_WAIT_NANOS) {
				backoff = MAX_WAIT_NANOS;
			}
			continue;
		}
		if (R_FAILED(res)) {
			logln(ERROR, "%ld", res);
			return;
		} else {
			logln(ERROR, "Check: bad status code %ld", res);
			return;
		}
	}

	snprintf(url, URL_SIZE, "%s/data/download", BASE_URL);
	char filename[200];
	struct tm now_tm;
	if (!gmtime_r(&now, &now_tm)) {
		logln(ERROR, "PANIC: gmtime failed");
		return;
	}
	n_strftime(filename, 200, "sdmc:/netpass_export_%Y%m%dT%H%M%S.zip", &now_tm);
	logln(INFO, "Downloading...");
	res = httpRequest("GET", url, 0, 0, (void*)1, filename, 0);
	if (res != 200) {
		logln(ERROR, "Download: bad status code %ld", res);
		remove(filename);
		return;
	}

	logln(INFO, "Successfully downloaded data export!");
	logln(INFO, "File stored at %s\n", filename);
#undef URL_SIZE
}
