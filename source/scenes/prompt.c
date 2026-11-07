/**
 * NetPass
 * Copyright (C) 2024 Sorunome
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

#include "prompt.h"

#define _data ((DataStruct*)sc->d)
#define _initdata ((InitData*)sc->data)
#define WIDTH_SCR 400
#define HEIGHT_SCR 240
#define MARGIN 20
#define WIDTH (WIDTH_SCR - 2*MARGIN)
#define HEIGHT (HEIGHT_SCR - 2*MARGIN)

typedef struct {
	union {
		C2D_Font font;
		LanguageString* lang_string;
	};
	char* message;
	Scene* ok;
	Scene* cancel;
	LanguageString* ok_string;
	LanguageString* cancel_string;
} InitData;

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_prompt;
	C2D_Text g_a_ok;
	C2D_Text g_b_back;
} DataStruct;

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	if (_initdata->message) {
		_data->g_staticBuf = C2D_TextBufNew(strlen(_initdata->message) + STR_A_OK_LEN + STR_B_GO_BACK_LEN);
		C2D_TextFontParse(&_data->g_prompt, _initdata->font, _data->g_staticBuf, _initdata->message);
		C2D_TextOptimize(&_data->g_prompt);
	} else {
		_data->g_staticBuf = C2D_TextBufNew(strlen(_s(*_initdata->lang_string)) + STR_A_OK_LEN + STR_B_GO_BACK_LEN);
		TextLangParse(&_data->g_prompt, _data->g_staticBuf, *_initdata->lang_string);
	}
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, *_initdata->ok_string);
	TextLangParse(&_data->g_b_back, _data->g_staticBuf, *_initdata->cancel_string);
}

static void render(Scene* sc) {
	C2D_DrawRectSolid(MARGIN, MARGIN, 0, WIDTH, HEIGHT, C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF));
	renderPlainTextFlags(&_data->g_prompt, C2D_WordWrap, MARGIN + 5, MARGIN + 5, 0.7f, 0, (WIDTH - MARGIN - 5) * 1.f);
	renderPlainText(&_data->g_b_back, MARGIN + 5, MARGIN + HEIGHT - 30, 1, 0);
	renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, MARGIN + WIDTH - 5, MARGIN + HEIGHT - 30, 1, 0);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		if (_initdata->message) free(_initdata->message);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
	if (_initdata->ok && _initdata->ok->need_free) {
		free(_initdata->ok);
		_initdata->ok = 0;
	}
	if (_initdata->cancel && _initdata->cancel->need_free) {
		free(_initdata->cancel);
		_initdata->cancel = 0;
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & KEY_A) {
		if (_initdata->ok) {
			sc->next_scene = _initdata->ok;
			_initdata->ok = 0; // something else is handling the freeing now
			return scene_switch;
		}
		return scene_pop;
	}
	if (kDown & KEY_B) {
		if (_initdata->cancel) {
			sc->next_scene = _initdata->cancel;
			_initdata->cancel = 0;
			return scene_switch;
		}
		return scene_pop;
	}
	return scene_continue;
}

Scene* getPromptScene(LanguageString* s, Scene* success) {
	Scene* scene = createScene(sizeof(InitData));
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	scene->is_popup = true;
	((InitData*)scene->data)->lang_string = s;
	((InitData*)scene->data)->ok = success;
	((InitData*)scene->data)->ok_string = &str_a_ok;
	((InitData*)scene->data)->cancel_string = &str_b_go_back;
	return scene;
}

Scene* getPromptSceneStr(char* s, C2D_Font font, Scene* success) {
	return getPromptSceneStrCustomMsg(s, font, success, &str_a_ok, 0, &str_b_go_back);
}

Scene* getPromptSceneStrCustomMsg(char* s, C2D_Font font, Scene* ok, LanguageString* ok_string, Scene* cancel, LanguageString* cancel_string) {
	Scene* scene = createScene(sizeof(InitData));
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	scene->is_popup = true;
	((InitData*)scene->data)->message = s;
	((InitData*)scene->data)->font = font;
	((InitData*)scene->data)->ok = ok;
	((InitData*)scene->data)->ok_string = ok_string;
	((InitData*)scene->data)->cancel = cancel;
	((InitData*)scene->data)->cancel_string = cancel_string;
	return scene;
}
