/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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

#include "bad_os_version.h"
#define _data ((DataStruct*)sc->d)
#define TEXT_BUF_LEN (STR_BAD_OS_VERSION_LEN)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text text;
} DataStruct;

static void init(Scene* sc) {
	sc->d = malloc(sizeof(DataStruct));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	TextLangParse(&_data->text, _data->g_staticBuf, str_bad_os_version);
}

static void render(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->text, C2D_AlignLeft | C2D_WordWrap, 10, 10, 0, 0.7, 0.7, 369.);
}

static void exit_scene(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
}

static SceneResult process(Scene* sc) {
	hidScanInput();
	if (hidKeysDown()) {
		return scene_stop;
	}
	return scene_continue;
}

Scene* getBadOsVersionScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = init;
	scene->render_top = render;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
