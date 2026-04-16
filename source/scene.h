/**
 * NetPass
 * Copyright (C) 2024-2026 Sorunome
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

#pragma once

#include <3ds.h>
#include <citro2d.h>

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240

typedef enum {scene_stop, scene_continue, scene_switch, scene_push, scene_pop} SceneResult;

typedef struct Scene Scene;

struct Scene {
	void (*init)(Scene*);
	void (*render_top)(Scene*);
	void (*render_bottom)(Scene*);
	void (*exit)(Scene*);
	SceneResult (*process)(Scene*);
	Scene* next_scene;
	Scene* pop_scene;
	u32 data;

	void* d;
	bool is_popup;
	bool need_free;
};

Scene* processScene(Scene* scene);
Scene* createScene(size_t data_size);
void renderTopScene(Scene* scene);
void renderBottomScene(Scene* scene);

#include "api.h"
#include "cecd.h"
#include "boss.h"
#include "render.h"
#include "strings.h"
#include "config.h"
#include "utils.h"

#include "scenes/about.h"
#include "scenes/back_alley.h"
#include "scenes/bad_os_version.h"
#include "scenes/download_progress.h"
#include "scenes/error.h"
#include "scenes/home.h"
#include "scenes/info.h"
#include "scenes/integration_scene.h"
#include "scenes/loading.h"
#include "scenes/location.h"
#include "scenes/misc_settings.h"
#include "scenes/prompt.h"
#include "scenes/report_entry.h"
#include "scenes/report_list.h"
#include "scenes/restart.h"
#include "scenes/scan_qr.h"
#include "scenes/set_time.h"
#include "scenes/settings.h"
#include "scenes/stop.h"
#include "scenes/switch.h"
#include "scenes/toggle_titles.h"
#include "scenes/update_patches.h"
#include "scenes/welcome.h"
