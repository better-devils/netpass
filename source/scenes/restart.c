/**
 * NetPass
 * Copyright (C) 2026 Sorunome
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

#include "restart.h"

extern char* filename_3dsx;

static void init(Scene* sc) {
	sc->d = 0;
}
static void exit_scene(Scene* sc) { }

static SceneResult process(Scene* sc) {
	if (sc->d) {
		svcSleepThread(100000000);
		return scene_continue;
	}
	if (filename_3dsx) return scene_stop;
	
	sc->d = (void*)1;
	const u64 title_id = 0x000400000F657400ull;
	Result res = _e(APT_PrepareToDoApplicationJump(0, title_id, get_title_destination(title_id)));
	if (R_FAILED(res)) return scene_continue;
	u8 param[0x300];
	u8 hmac[0x20];
	_e(APT_DoApplicationJump(param, sizeof(param), hmac));
	return scene_continue;
}

Scene* getRestartScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
