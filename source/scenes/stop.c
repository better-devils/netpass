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

#include "stop.h"

static void init(Scene* sc) { }
static void exit_scene(Scene* sc) { }

static SceneResult process(Scene* sc) {
	return scene_stop;
}

Scene* getStopScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = init;
	scene->exit = exit_scene;
	scene->process = process;
	return scene;
}
