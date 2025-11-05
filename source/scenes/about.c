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

#include "about.h"
#define N(x) scenes_about_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_B_GO_BACK_LEN + STR_ABOUT_LEAD_DEV_LEN + STR_ABOUT_REPORTS_LEN + STR_ABOUT_GRAPHICS_LEN + STR_ABOUT_MUSIC_LEN + STR_ABOUT_LOCALISATION_LEN + STR_ABOUT_NETPASS_COMMUNITY_LEN + STR_ABOUT_PRODUCTION_CAT_LEN + STR_ABOUT_SPECIAL_THANKS_LEN + STR_ABOUT_SPECIAL_THANKS_TXT_LEN)
#define NUM_CREDIT_CATAGORIES 7

typedef struct {
	const LanguageString* name;
	const int height;
	const char* entries;
	const LanguageString* lang_entries;
} N(RawCategory);

const N(RawCategory) N(raw_credits)[NUM_CREDIT_CATAGORIES] = {
	{&str_about_lead_dev, 1, "Sorunome", 0},
	{&str_about_reports, 1, "gart, checkraisefold, Sorunome", 0},
	{&str_about_graphics, 2, "Iveurne, DaGrand39, 24blueroses, KingMayro, MilesTheCreator, Lyril", 0},
	{&str_about_music, 1, "Meowbops, Lev, Batteries (Naomi)", 0},
	{&str_about_localisation, 1, 0, &str_about_netpass_community},
	{&str_about_production_cat, 1, "Laura", 0},
	{&str_about_special_thanks, 2, 0, &str_about_special_thanks_txt},
};

typedef struct {
	C2D_Text name;
	int height;
	C2D_Text entries;
} N(Category);

typedef struct {
	C2D_TextBuf g_staticBuf;
	N(Category) credits[NUM_CREDIT_CATAGORIES];
	int y_offset;
	C2D_Text netpass_website;
	C2D_Text go_back;
	float website_x;
	C2D_SpriteSheet laura;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->y_offset = 0;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN + 150);
	for (int i = 0; i < NUM_CREDIT_CATAGORIES; i++) {
		TextLangParse(&_data->credits[i].name, _data->g_staticBuf, *N(raw_credits)[i].name);
		if (N(raw_credits)[i].entries) {
			C2D_TextParse(&_data->credits[i].entries, _data->g_staticBuf, N(raw_credits)[i].entries);
		} else {
			TextLangParse(&_data->credits[i].entries, _data->g_staticBuf, *N(raw_credits)[i].lang_entries);
		}
		_data->credits[i].height = N(raw_credits)[i].height;
	}
	C2D_TextParse(&_data->netpass_website, _data->g_staticBuf, "https://netpass.cafe");
	TextLangParse(&_data->go_back, _data->g_staticBuf, str_b_go_back);
	float width;
	get_text_dimensions(&_data->netpass_website, 0.7, 0.7, &width, 0);
	_data->website_x = (SCREEN_TOP_WIDTH - width) / 2;
	_data->laura = C2D_SpriteSheetLoad("romfs:/gfx/laura.t3x");
}

void N(render)(Scene* sc) {
	if (!_data) return;
	int ycursor = 2 + _data->y_offset;
	C2D_DrawText(&_data->go_back, C2D_AlignLeft, 10, ycursor, 0, 0.5, 0.5);
	ycursor += 14;
	for (int i = 0; i < NUM_CREDIT_CATAGORIES; i++) {
		C2D_DrawText(&_data->credits[i].name, C2D_AlignLeft, 10, ycursor, 0, 0.7, 0.7);
		ycursor += 21;
		C2D_DrawText(&_data->credits[i].entries, C2D_AlignLeft | C2D_WordWrap, 40, ycursor, 0, 0.5, 0.5, 350.);
		ycursor += 14*_data->credits[i].height;
	}
	ycursor += 20;
	C2D_DrawText(&_data->netpass_website, C2D_AlignLeft, _data->website_x, ycursor, 0, 0.7, 0.7);
	ycursor += 300;
	C2D_Image img = C2D_SpriteSheetGetImage(_data->laura, 0);
	C2D_DrawImageAt(img, 0, ycursor, 0, NULL, 1, 1);
}

void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		C2D_SpriteSheetFree(_data->laura);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	if (_data) {
		_data->y_offset += ((kHeld & KEY_UP || kHeld & KEY_CPAD_UP) - ((kHeld & KEY_DOWN || kHeld & KEY_CPAD_DOWN) && 1))*2;
	}
	return scene_continue;
}

Scene* getAboutScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render_top = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	return scene;
}
