/**
 * NetPass
 * Copyright (C) 2025-2026 Sorunome
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

#include "report_entry.h"
#include "../report.h"
#include "../curl-handler.h"
#include <stdlib.h>
#include <malloc.h>
#define _data ((DataStruct*)sc->d)
#define _initdata ((InitData*)sc->data)
#define SETUP_EXDATA_ARR_INIT(a, x, y) if (!entry->data) break; \
	a* entry_data = (a*)entry->data; \
	_data->extra_data[i] = malloc(sizeof(x) + (sizeof(y) * entry_data->count)); \
	if (!_data->extra_data[i]) break; \
	x* ex_data = _data->extra_data[i]; \
	memset(ex_data, 0, sizeof(x) + (sizeof(y) * entry_data->count));
#define SETUP_EXDATA_INIT(a, x) if (!entry->data) break; \
	a* entry_data = (a*)entry->data; \
	_data->extra_data[i] = malloc(sizeof(x)); \
	if (!_data->extra_data[i]) break; \
	x* ex_data = _data->extra_data[i]; \
	memset(ex_data, 0, sizeof(x));
#define SETUP_EXDATA_RENDER(x) x* ex_data = _data->extra_data[i]; \
	if (!ex_data) break;

typedef struct {
	u32 num_thumbs;
	C2D_Image thumbs[];
} ExtraDataSwapdoodle;

typedef struct {
	C2D_Image pane[4];
} ExtraDataLetterbox;

typedef struct {
	C2D_Text greeting;
} ExtraDataMarioKart7;

typedef struct {
	C2D_Text last_game;
	C2D_Text country;
	C2D_Text greeting;
	C2D_Text custom_message;
	C2D_Text custom_reply;
} ExtraDataMiiPlaza;

typedef struct {
	C2D_Text island_name;
} ExtraDataTomodachiLife;

typedef struct {
	u64 id;
	u32 misc_id;
	ReportType report_type;
	char name[25];
} InitData;

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	u32 title_ids[12];
	C2D_Text* g_game_names;
	C2D_Text* g_mii_names;
	ReportMessages* msgs;
	int y_offset;
	void* extra_data[12];
	C2D_Text go_back;
	C2D_Text source_name;
} DataStruct;

static char* send_msg;
static u64 send_id;
static u32 send_misc_id;
static ReportType send_report_type;

static SceneResult report(Scene* sc) {
	static const int msgmaxlen = 200;
	send_msg = malloc(msgmaxlen + 1);
	SwkbdResult button;
	{
		char hint_text[STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN];
		snprintf(hint_text, STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN, _s(str_report_user_hint), _initdata->name);
		SwkbdState swkbd;
		memset(send_msg, 0, msgmaxlen + 1);
		swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, msgmaxlen);
		swkbdSetHintText(&swkbd, hint_text);
		swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, _s(str_cancel), false);
		swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, _s(str_submit), true);
		swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN | SWKBD_MULTILINE);
		swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
		button = swkbdInputText(&swkbd, send_msg, msgmaxlen + 1);
	}
	if (button == SWKBD_D1_CLICK1) {
		// successfully submitted the input
		send_id = _initdata->id;
		send_misc_id = _initdata->misc_id;
		send_report_type = _initdata->report_type;
		logln(INFO, "Got report: \"%s\", sending...", send_msg);
		Scene* scene = getLoadingScene(0, lambda(void, (void) {
			ReportSendPayload* data = malloc(sizeof(ReportSendPayload));
			if (!data) {
				_e(ERROR_OUT_OF_MEMORY);
				goto exit;
			}
			
			data->magic = 0x5053524e;
			data->version = 2;
			data->id = send_id;
			data->misc_id = send_misc_id;
			data->report_type = send_report_type;
			memcpy(data->msg, send_msg, sizeof(data->msg));

			char url[50];
			snprintf(url, 50, "%s/report/new2", BASE_URL);
			Result res = _e(httpRequest("POST", url, sizeof(ReportSendPayload), (u8*)data, 0, 0));
			free(data);
			if (R_FAILED(res)) {
				logln(ERROR, "Error sending report: %ld", res);
				goto exit;
			}

			logln(INFO, "report sent\n");
		exit:
			free(send_msg);
		}));
		scene->pop_scene = sc->pop_scene;
		sc->next_scene = scene;
		return scene_switch;
	}
	free(send_msg);
	return scene_pop;
}

static void exit_scene(Scene* sc);

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	memset(sc->d, 0, sizeof(DataStruct));
	if (R_FAILED(_e(loadReportMessages(&_data->msgs, _initdata->id, _initdata->misc_id, _initdata->report_type)))) {
		return exit_scene(sc);
	}

	_data->g_game_names = malloc(sizeof(C2D_Text) * _data->msgs->count);
	if (!_data->g_game_names) {
		_e_errno();
		return exit_scene(sc);
	}

	_data->g_mii_names = malloc(sizeof(C2D_Text) * _data->msgs->count);
	if (!_data->g_mii_names) {
		_e_errno();
		return exit_scene(sc);
	}

	_data->g_staticBuf = C2D_TextBufNew(300 * (_data->msgs->count + 1));

	// first create the heading
	{
		char render_text[STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN];
		snprintf(render_text, STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN, _s(str_report_user_hint), _initdata->name);
		C2D_TextFontParse(&_data->g_title, _font(str_report_user_hint), _data->g_staticBuf, render_text);
	}
	_data->y_offset = 0;
	TextLangParse(&_data->go_back, _data->g_staticBuf, str_b_go_back);
	if (_data->msgs->source_name) {
		char name[50];
		snprintf(name, 50, _s(str_report_source), _data->msgs->source_name);
		C2D_TextFontParse(&_data->source_name, _font(str_report_source), _data->g_staticBuf, name);
	} else {
		TextLangParse(&_data->source_name, _data->g_staticBuf, str_report_source_unknown);
	}

	for (int i = 0; i < _data->msgs->count; i++) {
		ReportMessagesEntry* entry = &_data->msgs->entries[i];
		_data->extra_data[i] = 0;
		if (entry->mii) {
			char render_text[STR_REPORT_MII_NAME_LEN + MII_UTF8_NAME_LEN];
			u8 mii_name[MII_UTF8_NAME_LEN];
			get_mii_name(mii_name, entry->mii);
			snprintf(render_text, STR_REPORT_MII_NAME_LEN + MII_UTF8_NAME_LEN, _s(str_report_mii_name), mii_name);
			C2D_TextFontParse(&_data->g_mii_names[i], _font(str_report_mii_name), _data->g_staticBuf, render_text);
		}
		if (entry->name) {
			C2D_TextParse(&_data->g_game_names[i], _data->g_staticBuf, entry->name);
		} else {
			char game_name[50];
			snprintf(game_name, 50, "%08lx", entry->title_id);
			C2D_TextParse(&_data->g_game_names[i], _data->g_staticBuf, game_name);
		}
		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				SETUP_EXDATA_INIT(ReportMessageEntryLetterBox, ExtraDataLetterbox);
				u8* jpegs = entry_data->jpegs;
				for (int j = 0; j < 4; j++) {
					u32 size = *(u32*)jpegs;
					jpegs += 4;
					if (size < 5000) { // protective measure
						if (!loadJpeg(&ex_data->pane[j], jpegs, size)) {
							ex_data->pane[j].tex = 0;
						}
					}
					jpegs += size;
					if (size % 4) jpegs += 4 - (size % 4);
					if (jpegs - entry_data->jpegs >= entry_data->jpeg_size) break;
				}
				break;
			}
			case TITLE_MARIO_KART_7: {
				SETUP_EXDATA_INIT(ReportMessageEntryMarioKart7, ExtraDataMarioKart7);
				char render_text[50];
				snprintf(render_text, 50, _s(str_report_mario_kart_7_greeting), entry_data->greeting);
				C2D_TextFontParse(&ex_data->greeting, _font(str_report_mario_kart_7_greeting), _data->g_staticBuf, render_text);
				break;
			}
			case TITLE_MII_PLAZA: {
				SETUP_EXDATA_INIT(ReportMessageEntryMiiPlaza, ExtraDataMiiPlaza);
				char render_text[100];
				snprintf(render_text, 100, _s(str_report_mii_plaza_last_game), entry_data->last_game);
				C2D_TextFontParse(&ex_data->last_game, _font(str_report_mii_plaza_last_game), _data->g_staticBuf, render_text);
				snprintf(render_text, 100, _s(str_report_mii_plaza_country), entry_data->country, entry_data->region);
				C2D_TextFontParse(&ex_data->country, _font(str_report_mii_plaza_country), _data->g_staticBuf, render_text);
				snprintf(render_text, 100, _s(str_report_mii_plaza_greeting), entry_data->greeting);
				C2D_TextFontParse(&ex_data->greeting, _font(str_report_mii_plaza_greeting), _data->g_staticBuf, render_text);
				if (entry_data->custom_message[0]) {
					snprintf(render_text, 100, _s(str_report_mii_plaza_custom_message), entry_data->custom_message);
					C2D_TextFontParse(&ex_data->custom_message, _font(str_report_mii_plaza_custom_message), _data->g_staticBuf, render_text);
					snprintf(render_text, 100, _s(str_report_mii_plaza_custom_reply), entry_data->custom_reply);
					C2D_TextFontParse(&ex_data->custom_reply, _font(str_report_mii_plaza_custom_reply), _data->g_staticBuf, render_text);
				}
				break;
			}
			case TITLE_TOMODACHI_LIFE: {
				SETUP_EXDATA_INIT(ReportMessageEntryTomodachiLife, ExtraDataTomodachiLife);
				char render_text[50];
				snprintf(render_text, 50, _s(str_report_tomodachi_life_island_name), entry_data->island_name);
				C2D_TextFontParse(&ex_data->island_name, _font(str_report_tomodachi_life_island_name), _data->g_staticBuf, render_text);
				break;
			}
			case TITLE_SWAPDOODLE: {
				SETUP_EXDATA_ARR_INIT(ReportMessageEntrySwapdoodle, ExtraDataSwapdoodle, C2D_Image);
				ex_data->num_thumbs = entry_data->count;
				for (int j = 0; j < entry_data->count; j++) {
					if (entry_data->thumbs[j].size < 5000) {
						if (!loadJpeg(&ex_data->thumbs[j], entry_data->thumbs[j].data, entry_data->thumbs[j].size)) {
							ex_data->thumbs[j].tex = 0;
						}
					}
				}
				break;
			}
		}
	}
}

static void render(Scene* sc) {
	if (!_data) {
		return;
	}
	int ycursor = 2 + _data->y_offset;
	C2D_DrawText(&_data->go_back, C2D_AlignLeft, 10, ycursor, 0, 0.5, 0.5);
	ycursor += 14;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, ycursor, 0, 1, 1);
	ycursor += 28;
	C2D_DrawText(&_data->source_name, C2D_AlignLeft, 10, ycursor, 0, 0.5, 0.5);
	ycursor += 18;
	for (int i = 0; i < _data->msgs->count; i++) {
		ReportMessagesEntry* entry = &_data->msgs->entries[i];
		C2D_DrawText(&_data->g_game_names[i], C2D_AlignLeft, 20, ycursor, 0, 0.5, 0.5);
		ycursor += 14;
		if (entry->mii) {
			C2D_DrawText(&_data->g_mii_names[i], C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
			ycursor += 14;
		}
		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				SETUP_EXDATA_RENDER(ExtraDataLetterbox);
				for (int j = 0; j < 4; j++) {
					if (!ex_data->pane[j].tex) continue;
					C2D_DrawImageAt(ex_data->pane[j], 40 + (82 * j), ycursor, 0, NULL, 1, 1);
				}
				ycursor += 50;
				break;
			}
			case TITLE_MARIO_KART_7: {
				SETUP_EXDATA_RENDER(ExtraDataMarioKart7);
				C2D_DrawText(&ex_data->greeting, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
				ycursor += 14;
				break;
			}
			case TITLE_MII_PLAZA: {
				SETUP_EXDATA_RENDER(ExtraDataMiiPlaza);
				C2D_DrawText(&ex_data->last_game, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
				ycursor += 14;
				C2D_DrawText(&ex_data->country, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
				ycursor += 14;
				C2D_DrawText(&ex_data->greeting, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
				ycursor += 14;
				if (*(u8*)&ex_data->custom_message) {
					C2D_DrawText(&ex_data->custom_message, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
					ycursor += 14;
					C2D_DrawText(&ex_data->custom_reply, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
					ycursor += 14;
				}
				break;
			}
			case TITLE_TOMODACHI_LIFE: {
				SETUP_EXDATA_RENDER(ExtraDataTomodachiLife);
				C2D_DrawText(&ex_data->island_name, C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
				ycursor += 14;
				break;
			}
			case TITLE_SWAPDOODLE: {
				SETUP_EXDATA_RENDER(ExtraDataSwapdoodle);
				for (int j = 0; j < ex_data->num_thumbs; j++) {
					if (!ex_data->thumbs[j].tex) continue;
					C2D_DrawImageAt(ex_data->thumbs[j], 40 + (82 * j), ycursor, 0, NULL, 1, 1);
				}
				ycursor += 50;
				break;
			}
		}
		ycursor += 3; // bottom padding
	}
}

static void exit_scene(Scene* sc) {
	if (!_data) return;
	for (int i = 0; i < 12; i++) {
		if (!_data->extra_data[i]) continue;
		ReportMessagesEntry* entry = &_data->msgs->entries[i];
		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				ExtraDataLetterbox* ex_data = _data->extra_data[i];
				for (int j = 0; j < 4; j++) {
					if (!ex_data->pane[j].tex) continue;
					C2D_ImageDelete(&ex_data->pane[j]);
				}
				break;
			}
			case TITLE_SWAPDOODLE: {
				ExtraDataSwapdoodle* ex_data = _data->extra_data[i];
				for (int j = 0; j < ex_data->num_thumbs; j++) {
					if (!ex_data->thumbs[j].tex) continue;
					C2D_ImageDelete(&ex_data->thumbs[j]);
				}
				break;
			}
		}
		free(_data->extra_data[i]);
	}
	if (_data->g_staticBuf) C2D_TextBufDelete(_data->g_staticBuf);
	if (_data->g_mii_names) free(_data->g_mii_names);
	if (_data->g_game_names) free(_data->g_game_names);
	if (_data->msgs) {
		freeReportMessages(_data->msgs);
		free(_data->msgs);
	}
	free(_data);
	sc->d = 0;
}

static SceneResult process(Scene* sc) {
	if (!_data) return scene_pop;
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	if (kDown & KEY_A) {
		if (_data->msgs->source_id == 0x504E) { // "NP"
			return report(sc);
		} else {
			sc->next_scene = getInfoScene(str_report_integration);
			return scene_push;
		}
	}
	_data->y_offset += ((kHeld & KEY_UP || kHeld & KEY_CPAD_UP) - ((kHeld & KEY_DOWN || kHeld & KEY_CPAD_DOWN) && 1))*2;
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getReportEntryScene(u64 mac, u32 transfer_id, char name[25], ReportType report_type) {
	Scene* scene = createScene(sizeof(InitData));
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	((InitData*)scene->data)->id = mac;
	((InitData*)scene->data)->misc_id = transfer_id;
	((InitData*)scene->data)->report_type = report_type;
	memcpy(((InitData*)scene->data)->name, name, 25);
	return scene;
}
