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
#define WIDTH_SCR 400
#define HEIGHT_SCR 240
#define MARGIN 20
#define WIDTH (WIDTH_SCR - 2*MARGIN)
#define HEIGHT (HEIGHT_SCR - 2*MARGIN)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_prompt;
	C2D_Text g_a_ok;
	C2D_Text g_b_back;
	char* message;
} DataStruct;

static void init(Scene* sc) {
	C2D_Font font = sc->d;
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	if (font) {
		font = (void*)((u32)font & 0xFFFFFFFE);
		_data->message = (void*)sc->data;
		_data->g_staticBuf = C2D_TextBufNew(strlen(_data->message) + STR_A_OK_LEN + STR_B_GO_BACK_LEN);
		C2D_TextFontParse(&_data->g_prompt, font, _data->g_staticBuf, (void*)sc->data);
		C2D_TextOptimize(&_data->g_prompt);
	} else {
		_data->message = 0;
		_data->g_staticBuf = C2D_TextBufNew(strlen(_s((void*)sc->data)) + STR_A_OK_LEN + STR_B_GO_BACK_LEN);
		TextLangParse(&_data->g_prompt, _data->g_staticBuf, (void*)sc->data);
	}
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, str_a_ok);
	TextLangParse(&_data->g_b_back, _data->g_staticBuf, str_b_go_back);
}

static void render(Scene* sc) {
	C2D_DrawRectSolid(MARGIN, MARGIN, 0, WIDTH, HEIGHT, C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF));
	renderPlainTextFlags(&_data->g_prompt, C2D_WordWrap, MARGIN + 5, MARGIN + 5, 0.7f, 0, (WIDTH - MARGIN - 5) * 1.f);
	renderPlainText(&_data->g_b_back, MARGIN + 5, MARGIN + HEIGHT - 30, 1, 0);
	renderPlainTextFlags(&_data->g_a_ok, C2D_AlignRight, MARGIN + WIDTH - 5, MARGIN + HEIGHT - 30, 1, 0);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		if (_data->message) free(_data->message);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & KEY_A) return scene_switch;
	if (kDown & KEY_B) {
		if (sc->next_scene->need_free) {
			free(sc->next_scene);
			sc->next_scene = 0;
		}
		return scene_pop;
	}
	return scene_continue;
}

Scene* getPromptScene(LanguageString s, Scene* success) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	scene->data = (u32)s;
	scene->next_scene = success;
	scene->is_popup = true;
	return scene;
}

Scene* getPromptSceneStr(char* s, C2D_Font font, Scene* success) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	scene->data = (u32)s;
	scene->d = (void*)((u32)font | 1);
	scene->next_scene = success;
	scene->is_popup = true;
	return scene;
}
