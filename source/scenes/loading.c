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

#include "loading.h"
#include <stdlib.h>
#include "../render.h"
#define _data ((DataStruct*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_loading;
	C2D_Text g_dots;
	C2D_SpriteSheet spr;
	float text_x;
	float text_y;
	float text_width;
	Thread thread;
	bool thread_done;
} DataStruct;

static void threadFn(Scene* sc) {
	((void(*)(void))(sc->data))();
	_data->thread_done = true;
}

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(STR_LOADING_LEN + 3);
	TextLangParse(&_data->g_loading, _data->g_staticBuf, str_loading);
	C2D_TextParse(&_data->g_dots, _data->g_staticBuf, "...");
	float height;
	get_text_dimensions(&_data->g_loading, 1, 1, &_data->text_width, &height);
	_data->text_x = (SCREEN_TOP_WIDTH - _data->text_width) / 2;
	_data->text_y = (SCREEN_TOP_HEIGHT - height) / 2;

	_data->spr = C2D_SpriteSheetLoad("romfs:/gfx/loading.t3x");

	_data->thread_done = false;
	_data->thread = threadCreate((void(*)(void*))threadFn, sc, 8*1024, main_thread_prio()-1, -2, false);
}

static void render(Scene* sc) {
	if (!_data) return;
	C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, 0);
	C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
	renderText(&_data->g_loading, _data->text_x, _data->text_y, 1, 0);
	renderText(&_data->g_dots, _data->text_x + _data->text_width - 35 + 10*(time(NULL)%2), _data->text_y, 1, 0);
}


static void exit_scene(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		if (_data->spr) C2D_SpriteSheetFree(_data->spr);
		threadJoin(_data->thread, U64_MAX);
		threadFree(_data->thread);
		free(_data);
	}
}

static SceneResult process(Scene* sc) {
	if (_data && _data->thread_done) {
		if (sc->next_scene) return scene_switch;
		return scene_pop;
	}
	return scene_continue;
}

Scene* getLoadingScene(Scene* next_scene, void(*func)(void)) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	scene->next_scene = next_scene;
	scene->data = (u32)func;
	return scene;
}
