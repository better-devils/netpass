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

#include "report_entry.h"
#include "../report.h"
#include "../curl-handler.h"
#include <stdlib.h>
#include <malloc.h>
#define N(x) scenes_report_entry_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define SETUP_EXDATA_INIT(a, x) if (!entry->data) break; \
	a* entry_data = (a*)entry->data; \
	_data->extra_data[i] = malloc(sizeof(N(x))); \
	if (!_data->extra_data[i]) break; \
	N(x)* ex_data = _data->extra_data[i]; \
	memset(ex_data, 0, sizeof(N(x)));
#define SETUP_EXDATA_RENDER(x) N(x)* ex_data = _data->extra_data[i]; \
	if (!ex_data) break;

typedef struct {
	C2D_Image pane[4];
} N(ExtraDataLetterbox);

typedef struct {
	C2D_Text greeting;
} N(ExtraDataMarioKart7);

typedef struct {
	C2D_Text last_game;
	C2D_Text country;
	C2D_Text greeting;
	C2D_Text custom_message;
	C2D_Text custom_reply;
} N(ExtraDataMiiPlaza);

typedef struct {
	C2D_Text island_name;
} N(ExtraDataTomodachiLife);

typedef struct {
	C2D_TextBuf g_staticBuf;
	ReportListEntry* entry;
	C2D_Text g_title;
	u32 title_ids[12];
	C2D_Text* g_game_names;
	C2D_Text* g_mii_names;
	ReportMessages* msgs;
	int y_offset;
	void* extra_data[12];
	C2D_Text go_back;
	C2D_Text source_name;
} N(DataStruct);

char* N(send_msg);
u32 N(send_transfer_id);

SceneResult N(report)(Scene* sc) {
	static const int msgmaxlen = 200;
	N(send_msg) = malloc(msgmaxlen + 1);
	SwkbdResult button;
	{
		char hint_text[STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN];
		u8 mii_name[MII_UTF8_NAME_LEN];
		get_mii_name(mii_name, &_data->entry->mii);
		snprintf(hint_text, STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN, _s(str_report_user_hint), mii_name);
		SwkbdState swkbd;
		memset(N(send_msg), 0, msgmaxlen + 1);
		swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, msgmaxlen);
		swkbdSetHintText(&swkbd, hint_text);
		swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, _s(str_cancel), false);
		swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, _s(str_submit), true);
		swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN | SWKBD_MULTILINE);
		swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
		button = swkbdInputText(&swkbd, N(send_msg), msgmaxlen + 1);
	}
	if (button == SWKBD_D1_CLICK1) {
		// successfully submitted the input
		N(send_transfer_id) = _data->entry->transfer_id;
		printf("Got report: \"%s\", sending...\n", N(send_msg));
		Scene* scene = getLoadingScene(0, lambda(void, (void) {
			CecMessageHeader msg;
			Result res = _e(reportGetSomeMsgHeader(&msg, N(send_transfer_id)));
			if (R_FAILED(res)) {
				printf("ERROR: %lx\n", res);
				goto exit;
			}
			SHA256_HASH hash;
			Sha256Calculate(&msg, 0x28, &hash);
			ReportSendPayload* data = malloc(sizeof(ReportSendPayload));
			if (!data) {
				_e(ERROR_OUT_OF_MEMORY);
				goto exit;
			}
			
			data->magic = 0x5053524e;
			data->version = 1;
			memcpy(data->message_id, msg.message_id, sizeof(CecMessageId));
			memcpy(&data->hash, &hash, sizeof(SHA256_HASH));
			memcpy(data->msg, N(send_msg), sizeof(data->msg));

			char url[50];
			snprintf(url, 50, "%s/report/new", BASE_URL);
			res = _e(httpRequest("POST", url, sizeof(ReportSendPayload), (u8*)data, 0, 0, 0));
			free(data);
			if (R_FAILED(res)) {
				printf("Error sending report: %ld\n", res);
				goto exit;
			}

			printf("report sent\n");
		exit:
			free(N(send_msg));
		}));
		scene->pop_scene = sc->pop_scene;
		sc->next_scene = scene;
		return scene_switch;
	}
	free(N(send_msg));
	return scene_pop;
}

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	memset(sc->d, 0, sizeof(N(DataStruct)));
	_data->entry = (ReportListEntry*)sc->data;

	_data->msgs = malloc(sizeof(ReportMessages));
	if (!_data->msgs) {
		free(_data);
		sc->d = NULL;
		return;
	}

	if (!loadReportMessages(_data->msgs, _data->entry->transfer_id)) {
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_game_names = malloc(sizeof(C2D_Text) * _data->msgs->count);
	if (!_data->g_game_names) {
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_mii_names = malloc(sizeof(C2D_Text) * _data->msgs->count);
	if (!_data->g_mii_names) {
		free(_data->g_game_names);
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_staticBuf = C2D_TextBufNew(300 * (_data->msgs->count + 1));

	// first create the heading
	{
		char render_text[STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN];
		u8 mii_name[MII_UTF8_NAME_LEN];
		get_mii_name(mii_name, &_data->entry->mii);
		snprintf(render_text, STR_REPORT_USER_HINT_LEN + MII_UTF8_NAME_LEN, _s(str_report_user_hint), mii_name);
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
				SETUP_EXDATA_INIT(u8, ExtraDataLetterbox);
				for (int j = 0; j < 4; j++) {
					u32 size = *((u32*)entry_data);
					entry_data += 4;
					if (size < 5000) { // protective measure
						if (!loadJpeg(&ex_data->pane[j], entry_data, size)) {
							ex_data->pane[j].tex = 0;
						}
					}
					entry_data += size;
					if (size % 4) entry_data += 4 - (size % 4);
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
		}
	}
}

void N(render)(Scene* sc) {
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
		}
		ycursor += 3; // bottom padding
	}
}

void N(exit)(Scene* sc) {
	if (_data) {
		for (int i = 0; i < 12; i++) {
			if (!_data->extra_data[i]) continue;
			ReportMessagesEntry* entry = &_data->msgs->entries[i];
			switch (entry->title_id) {
				case TITLE_LETTER_BOX: {
					N(ExtraDataLetterbox)* ex_data = _data->extra_data[i];
					for (int j = 0; j < 4; j++) {
						if (!ex_data->pane[j].tex) continue;
						C2D_ImageDelete(&ex_data->pane[j]);
					}
				}
			}
			free(_data->extra_data[i]);
		}
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data->g_mii_names);
		free(_data->g_game_names);
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	if (kDown & KEY_A) {
		if (_data->msgs->source_id == 0x504E) { // "NP"
			return N(report)(sc);
		} else {
			sc->next_scene = getInfoScene(str_report_integration);
			return scene_push;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	_data->y_offset += ((kHeld & KEY_UP || kHeld & KEY_CPAD_UP) - ((kHeld & KEY_DOWN || kHeld & KEY_CPAD_DOWN) && 1))*2;
	return scene_continue;
}

Scene* getReportEntryScene(ReportListEntry* entry) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->is_popup = false;
	scene->need_free = true;
	scene->data = (u32)(void*)entry;
	return scene;
}
