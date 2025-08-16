/**
 * NetPass
 * Copyright (C) 2024 Sorunome
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

#define N(x) scenes_prompt_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
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
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(2000);
	TextLangParse(&_data->g_prompt, _data->g_staticBuf, (void*)sc->data);
	TextLangParse(&_data->g_a_ok, _data->g_staticBuf, str_a_ok);
	TextLangParse(&_data->g_b_back, _data->g_staticBuf, str_b_go_back);
}

void N(render)(Scene* sc) {
	C2D_DrawRectSolid(MARGIN, MARGIN, 0, WIDTH, HEIGHT, C2D_Color32(0xCC, 0xCC, 0xCC, 0xFF));
	C2D_DrawText(&_data->g_prompt, C2D_AlignLeft | C2D_WordWrap, MARGIN + 5, MARGIN + 5, 0, 1, 1, (WIDTH - 2*MARGIN - 10) * 1.f);
	C2D_DrawText(&_data->g_b_back, C2D_AlignLeft, MARGIN + 5, MARGIN + HEIGHT - 30, 0, 1, 1);
	C2D_DrawText(&_data->g_a_ok, C2D_AlignRight, MARGIN + WIDTH - 5, MARGIN + HEIGHT - 30, 0, 1, 1);
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
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->data = (u32)s;
	scene->next_scene = success;
	scene->is_popup = true;
	scene->need_free = true;
	return scene;
}
