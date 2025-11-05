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

#include "download_progress.h"
#include <stdlib.h>
#include "../render.h"
#define N(x) scenes_download_progress_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define _initdata ((N(InitData)*)sc->data)
#define _reply (*(_initdata->reply))

typedef struct {
	CurlReply** reply;
	void(*func)(void);
} N(InitData);

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_loading;
	C2D_SpriteSheet spr;
	float text_x;
	float text_y;
	float text_width;
	Thread thread;
	bool thread_done;
} N(DataStruct);

void N(threadFn)(Scene* sc) {
	_initdata->func();
	_data->thread_done = true;
}

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(STR_DOWNLOADING_LEN);
	TextLangParse(&_data->g_loading, _data->g_staticBuf, str_downloading);
	float height;
	get_text_dimensions(&_data->g_loading, 1, 1, &_data->text_width, &height);
	_data->text_x = (SCREEN_TOP_WIDTH - _data->text_width) / 2;
	_data->text_y = (SCREEN_TOP_HEIGHT - height) / 2;

	_data->spr = C2D_SpriteSheetLoad("romfs:/gfx/loading.t3x");

	_data->thread_done = false;
	_data->thread = threadCreate((void(*)(void*))N(threadFn), sc, 8*1024, main_thread_prio()-1, -2, false);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, 0);
	C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
	renderText(&_data->g_loading, _data->text_x, _data->text_y - 40, 1, 0);
	C2D_DrawRectSolid(20, _data->text_y, 0, SCREEN_TOP_WIDTH - 40, 40, clr_netpass_green);
	float ratio = _reply && _reply->dltotal ? 1.f * _reply->dlnow / _reply->dltotal : 0.f;
	C2D_DrawRectSolid(22 + ratio * (SCREEN_TOP_WIDTH - 44), _data->text_y + 2, 0, (1.0 - ratio) * (SCREEN_TOP_WIDTH - 44), 36, clr_white);
}


void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		if (_data->spr) C2D_SpriteSheetFree(_data->spr);
		threadJoin(_data->thread, U64_MAX);
		threadFree(_data->thread);
		free(_initdata);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	if (_data && _data->thread_done) {
		if (sc->next_scene) return scene_switch;
		return scene_pop;
	}
	return scene_continue;
}

Scene* getDownloadProgressScene(CurlReply** reply, Scene* next_scene, void(*func)(void)) {
	Scene* scene = createScene(sizeof(N(InitData)));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render_top = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->next_scene = next_scene;
	((N(InitData)*)scene->data)->func = func;
	((N(InitData)*)scene->data)->reply = reply;
	return scene;
}
