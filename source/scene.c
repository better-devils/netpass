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

#include "scene.h"
#include <malloc.h>

void initScene(Scene* scene) {
	if (scene->d && !((u32)scene->d & 1)) return;
	scene->init(scene);
}

void exitScene(Scene* scene) {
	scene->exit(scene);
	scene->d = 0;
	if (scene->need_free) {
		free(scene);
	}
}

Scene* processScene(Scene* scene) {
	SceneResult res = scene->process(scene);
	switch (res) {
	case scene_continue:
	{
		return scene;
	}
	case scene_stop:
	{
		exitScene(scene);
		return 0;
	}
	case scene_switch:
	{
		Scene* new_scene = scene->next_scene;
		if (!new_scene) {
			logln(ERROR, "Could not create scene!!");
			return NULL;
		}
		if (!new_scene->pop_scene) {
			new_scene->pop_scene = scene->pop_scene;
		}
		exitScene(scene);
		initScene(new_scene);
		if (new_scene->pop_scene) {
			initScene(new_scene->pop_scene);
		}
		return new_scene;
	}
	case scene_push:
	{
		Scene* new_scene = scene->next_scene;
		new_scene->pop_scene = scene;
		initScene(new_scene);
		return new_scene;
	}
	case scene_pop:
	{
		Scene* new_scene = scene->pop_scene;
		exitScene(scene);
		if (new_scene) initScene(new_scene);
		return new_scene;
	}
	}
	return scene;
}

void renderTopScene(Scene* scene) {
	if (scene->render_top) {
		scene->render_top(scene);
	} else if (scene->pop_scene) {
		renderTopScene(scene->pop_scene);
	}
}

void renderBottomScene(Scene* scene) {
	if (scene->render_bottom) {
		scene->render_bottom(scene);
	} else if (scene->pop_scene) {
		renderBottomScene(scene->pop_scene);
	}
}

Scene* createScene(size_t data_size) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	memset(scene, 0, sizeof(Scene));
	scene->need_free = true;
	if (data_size) {
		void* data = malloc(data_size);
		if (!data) {
			free(scene);
			return NULL;
		}
		scene->data = (u32)data;
	}
	return scene;
}
