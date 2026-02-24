/**
 * NetPass
 * Copyright (C) 2024-2025 Sorunome
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

#include "back_alley.h"
#include "../utils.h"
#include "../api.h"
#include "../curl-handler.h"
#include "../music.h"
#include "../render.h"
#include "../image_cache.h"
#include <stdlib.h>
#include <time.h>
#define _data ((DataStruct*)sc->d)
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
	C2D_Image background;
	C2D_SpriteSheet spr;
	bool view_bg_only;
} DataStruct;

static Scene* current_scene;
static PlayCoins* play_coins;
static u32 buy_title_id;

static bool init_playcoins(Scene* sc);
static void load_paytext(C2D_Text* staticText, C2D_TextBuf staticBuf, int cost_amount);

static void refresh(Scene* sc) {
	if (_data->play_coins) free(_data->play_coins);
	if (!init_playcoins(sc)) return;
	load_paytext(&_data->g_paytext, _data->g_staticBuf, config.price > MAX_PRICE ? 0 : config.price);
}

static SceneResult buy_pass(Scene* sc, int i) {
	buy_title_id = _data->title_ids[i];
	play_coins = malloc(sizeof(PlayCoins));
	if (!play_coins) {
		return scene_continue;
	}
	current_scene = sc;
	memcpy(play_coins, _data->play_coins, sizeof(PlayCoins));

	Scene* scene = getLoadingScene(0, lambda(void, (void) {
		char url[80];
		snprintf(url, 80, "%s/pass/title_id/%lx", BASE_URL, buy_title_id);
		Result res = httpRequest("PUT", url, 0, 0, 0, 0);
		if (R_FAILED(res)) {
			if (res == -404) {
				logln(ERROR, "No fitting pass found!");
				free(play_coins);
				return;
			}
			goto error;
		}
		play_coins->total_coins -= config.price;
		config.price += 2;
		configWrite();
		if (config.price > 2) {
			Handle handle = 0;
			res = _e(FSUSER_OpenFile(&handle, sharedextdata_b, fsMakePath(PATH_ASCII, "/gamecoin.dat"), FS_OPEN_WRITE, 0));
			if (R_FAILED(res)) goto error;
			u32 tmpval=0;
			res = _e(FSFILE_Write(handle, &tmpval, 0, play_coins, sizeof(PlayCoins), FS_WRITE_FLUSH));
			FSFILE_Close(handle);
			if (R_FAILED(res)) goto error;
			free(play_coins);
		}
		triggerDownloadInboxes();
		refresh(current_scene);
		return;
	error:
		_e(res);
		logln(ERROR, "failed processing pass: %lx", res);
		free(play_coins);
	}));
	sc->next_scene = scene;
	return scene_push;
}

static void load_paytext(C2D_Text* staticText, C2D_TextBuf staticBuf, int cost_amount) {
	const char* s = _s(str_back_alley_pay);
	C2D_Font font = _font(str_back_alley_pay);
	char text[50];
	snprintf(text, 50, s, cost_amount);
	C2D_TextFontParse(staticText, font, staticBuf, text);
	C2D_TextOptimize(staticText);
}

static bool init_playcoins(Scene* sc) {
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

static void init_gamelist(Scene* sc) {
	_data->number_games = 0;
	NetpassTitleData* title_data = getTitleData();
	for (int i = 0; i < title_data->num_titles; i++) {
		if (isTitleIgnored(title_data->titles[i].title_id)) continue;
		_data->title_ids[_data->number_games] = title_data->titles[i].title_id;
		C2D_TextParse(&_data->g_game_titles[_data->number_games], _data->g_staticBuf, title_data->titles[i].name);
		C2D_TextOptimize(&_data->g_game_titles[_data->number_games]);
		_data->number_games++;
	}
}

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	memset(sc->d, 0, sizeof(DataStruct));
	
	if (!init_playcoins(sc)) return;
	get_background_image("back_alley", &_data->spr, &_data->background, false);
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + 12*24);
	init_gamelist(sc);
	
	getCurMusic(_data->prev_music);
	playMusic("back_alley");

	_data->cursor = 0;
	_data->view_bg_only = false;
	_data->show_games = false;
	TextLangParse(&_data->g_header, _data->g_staticBuf, str_back_alley);
	TextLangParse(&_data->g_subtext, _data->g_staticBuf, str_back_alley_message);
	load_paytext(&_data->g_paytext, _data->g_staticBuf, config.price > MAX_PRICE ? 0 : config.price);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);
}

static void render(Scene* sc) {
	if (!_data) return;
	if (_data->spr) {
		C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, 0);
		C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
	} else if (_data->background.tex) {
		C2D_DrawImageAt(_data->background, 0, 0, 0, NULL, 1, 1);
	}
	if (_data->view_bg_only) return;
	
	renderText(&_data->g_header, 10, 10, 1, 0);
	renderText(&_data->g_subtext, 11, 35, 0.5, 0);
	if (_data->show_games) {
		int i = 0;
		for (; i < _data->number_games; i++) {
			renderText(&_data->g_game_titles[i], 30, 55 + (i*14), 0.5, 0);
		}
		renderText(&_data->g_back, 30, 55 + (i*14), 0.5, 0);

		renderCursor(13, _data->cursor*14 + 55 + 1, 0.5);
	} else {
		bool grayed_out = config.price > MAX_PRICE || config.price > _data->play_coins->total_coins;
		renderText(&_data->g_paytext, 34, 55, 1, grayed_out ? clr_gray : 0);
		renderText(&_data->g_back, 34, 80, 1, 0);

		renderCursor(1, _data->cursor*25 + 55 + 2, 1);
	}
}

static void exit_scene(Scene* sc) {
	if (_data) {
		if (_data->spr) C2D_SpriteSheetFree(_data->spr);
		playMusic(_data->prev_music);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data->play_coins);
		free(_data);
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	if (_data) {
		_data->view_bg_only = (kHeld & KEY_L) || (kHeld & KEY_R);
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
					SceneResult result = buy_pass(sc, _data->cursor);
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
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
