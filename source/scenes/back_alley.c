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

#include "back_alley.h"
#include "../utils.h"
#include "../api.h"
#include "../curl-handler.h"
#include "../music.h"
#include <stdlib.h>
#include <time.h>
#define N(x) scenes_back_alley_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_BACK_ALLEY_PAY_LEN + STR_BACK_ALLEY_LEN + STR_BACK_ALLEY_MESSAGE_LEN + STR_BACK_LEN)
#define MAX_PRICE 10

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_header;
	C2D_Text g_subtext;
	C2D_Text g_paytext;
	C2D_Text g_game_titles[12];
	u32 title_ids[12];
	C2D_Text g_back;
	PlayCoins* play_coins;
	int cursor;
	int number_games;
	bool show_games;
	char prev_music[20];
} N(DataStruct);

PlayCoins* N(play_coins);
u32 N(buy_title_id);

SceneResult N(buy_pass)(Scene* sc, int i) {
	N(buy_title_id) = _data->title_ids[i];
	N(play_coins) = malloc(sizeof(PlayCoins));
	if (!N(play_coins)) {
		return scene_continue;
	}
	memcpy(N(play_coins), _data->play_coins, sizeof(PlayCoins));

	Scene* scene = getLoadingScene(0, lambda(void, (void) {
		char url[80];
		snprintf(url, 80, "%s/pass/title_id/%lx", BASE_URL, N(buy_title_id));
		Result res = httpRequest("PUT", url, 0, 0, 0, 0, 0);
		if (R_FAILED(res)) {
			if (res == -404) {
				printf("ERROR: No fitting pass found!");
				free(N(play_coins));
				return;
			}
			goto error;
		}
		N(play_coins)->total_coins -= config.price;
		config.price += 2;
		configWrite();
		if (config.price > 2) {
			Handle handle = 0;
			res = _e(FSUSER_OpenFile(&handle, sharedextdata_b, fsMakePath(PATH_ASCII, "/gamecoin.dat"), FS_OPEN_WRITE, 0));
			if (R_FAILED(res)) goto error;
			u32 tmpval=0;
			res = _e(FSFILE_Write(handle, &tmpval, 0, N(play_coins), sizeof(PlayCoins), FS_WRITE_FLUSH));
			FSFILE_Close(handle);
			if (R_FAILED(res)) goto error;
			free(N(play_coins));
		}
		triggerDownloadInboxes();
		return;
	error:
		_e(res);
		printf("ERROR: failed processing pass: %lx\n", res);
		free(N(play_coins));
	}));
	sc->next_scene = scene;
	return scene_push;
}

void N(load_paytext)(C2D_Text* staticText, C2D_TextBuf staticBuf, int cost_amount) {
	const char* s = _s(str_back_alley_pay);
	C2D_Font font = _font(str_back_alley_pay);
	char text[50];
	snprintf(text, 50, s, cost_amount);
	C2D_TextFontParse(staticText, font, staticBuf, text);
}

bool N(init_playcoins)(Scene* sc) {
	_data->play_coins = malloc(sizeof(PlayCoins));
	if (!_data->play_coins) {
		_e(ERROR_OUT_OF_MEMORY);
		free(_data);
		sc->d = 0;
		return false;
	}
	FILE* f = fopen("sharedextdata_b:/gamecoin.dat", "rb");
	if (!f) {
		_e_errno();
		free(_data->play_coins);
		free(_data);
		sc->d = 0;
		return false;
	}
	if (fread(_data->play_coins, sizeof(PlayCoins), 1, f) != 1) {
		_e_errno();
		fclose(f);
		free(_data->play_coins);
		free(_data);
		sc->d = 0;
		return false;
	}
	fclose(f);

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	if (tm.tm_year + 1900 != config.year || tm.tm_mon + 1 != config.month || tm.tm_mday != config.day) {
		// our config is from the past day, time to set things up for the new day!
		config.price = 0;
		config.year = tm.tm_year + 1900;
		config.month = tm.tm_mon + 1;
		config.day = tm.tm_mday;
		configWrite();
	}
	return true;
}

bool N(init_gamelist)(Scene* sc) {
	_data->number_games = 0;
	NetpassTitleData* title_data = getTitleData();
	for (int i = 0; i < title_data->num_titles; i++) {
		if (isTitleIgnored(title_data->titles[i].title_id)) continue;
		_data->title_ids[_data->number_games] = title_data->titles[i].title_id;
		C2D_TextParse(&_data->g_game_titles[_data->number_games], _data->g_staticBuf, title_data->titles[i].name);
		_data->number_games++;
	}
	return true;
}

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	
	if (!N(init_playcoins)(sc)) return;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + 12*24);
	if (!N(init_gamelist)(sc)) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data->play_coins);
		free(_data);
		sc->d = 0;
		return;
	}
	
	getCurMusic(_data->prev_music);
	playMusic("back_alley");

	_data->cursor = 0;
	_data->show_games = false;
	TextLangParse(&_data->g_header, _data->g_staticBuf, str_back_alley);
	TextLangParse(&_data->g_subtext, _data->g_staticBuf, str_back_alley_message);
	N(load_paytext)(&_data->g_paytext, _data->g_staticBuf, config.price > MAX_PRICE ? 0 : config.price);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	C2D_DrawText(&_data->g_header, C2D_AlignLeft | C2D_WithColor, 10, 10, 0, 1, 1, clr);
	C2D_DrawText(&_data->g_subtext, C2D_AlignLeft | C2D_WithColor, 11, 35, 0, 0.5, 0.5, clr);
	if (_data->show_games) {
		int i = 0;
		for (; i < _data->number_games; i++) {
			C2D_DrawText(&_data->g_game_titles[i], C2D_AlignLeft | C2D_WithColor, 30, 55 + (i*14), 0, 0.5, 0.5, clr);
		}
		C2D_DrawText(&_data->g_back, C2D_AlignLeft | C2D_WithColor, 30, 55 + (i*14), 0, 0.5, 0.5, clr);

		int x = 22;
		int y = _data->cursor*14 + 55 + 3;
		C2D_DrawTriangle(x, y, clr, x, y +10, clr, x + 8, y + 5, clr, 1);
	} else {
		bool grayed_out = config.price > MAX_PRICE || config.price > _data->play_coins->total_coins;
		C2D_DrawText(&_data->g_paytext, C2D_AlignLeft | C2D_WithColor, 30, 55, 0, 1, 1, grayed_out ? C2D_Color32(0, 0, 0, 0x80) : clr);
		C2D_DrawText(&_data->g_back, C2D_AlignLeft | C2D_WithColor, 30, 80, 0, 1, 1, clr);

		int x = 10;
		int y = _data->cursor*25 + 55 + 5;
		C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 0);
	}
}

void N(exit)(Scene* sc) {
	if (_data) {
		playMusic(_data->prev_music);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (_data) {
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->show_games) {
			if (_data->cursor < 0) _data->cursor = _data->number_games;
			if (_data->cursor > _data->number_games) _data->cursor = 0;
			if (kDown & KEY_A) {
				if (_data->cursor == _data->number_games) {
					// go back
					_data->cursor = 0;
					_data->show_games = false;
					return scene_continue;
				} else {
					// picked a game
					SceneResult result = N(buy_pass)(sc, _data->cursor);
					if (result == scene_push) {
						_data->cursor = 0;
						_data->show_games = false;
					}
					return result;
				}
			}
			if (kDown & KEY_B) {
				_data->cursor = 0;
				_data->show_games = false;
				return scene_continue;
			}
		} else {
			if (_data->cursor < 0) _data->cursor = 1;
			if (_data->cursor > 1) _data->cursor = 0;
			if (kDown & KEY_A) {
				if (_data->cursor == 0 && config.price <= MAX_PRICE && config.price <= _data->play_coins->total_coins) {
					_data->cursor = 0;
					_data->show_games = true;
					return scene_continue;
				}
				if (_data->cursor == 1) return scene_pop;
			}
			if (kDown & KEY_B) return scene_pop;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getBackAlleyScene() {
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
