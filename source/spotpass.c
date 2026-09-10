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

#include <libeedle.h>
#include "spotpass.h"
#include "strings.h"
#include "utils.h"
#include "api.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_NEWS_TASKS 5
#define CUSTOM_IMS_HEADER "3ds-if-modified-since"
#define NETPASS_ID (0xF6574)
#define FULL_NETPASS_ID ((u64)0x0004000000000000ull | (NETPASS_ID << 8))

Result setupSpotpass(bool is_3dsx) {
	// first init boss with our id
	// As we may be running via the homebrew menu we need our own id
	// and priv mode so that we can pretend we are always running as cia.
	// Because of homebrew we can do this! :D

	Result res;
	BossContext* ctx = NULL;
	Smdh* smdh = NULL;
	smdh = malloc(sizeof(Smdh));
	if (!smdh) return _e(ERROR_OUT_OF_MEMORY);
	
	
	res = _e(bossInit(FULL_NETPASS_ID, false));
	if (R_FAILED(res)) goto cleanup;

	// Now, setting up storage.
	{
		// first, we check if we need to set it up at all
		
		if (needSetupSpotpassExtData(FULL_NETPASS_ID)) {
			res = _e(setupSpotpassExtData(FULL_NETPASS_ID, smdh, 42, 42, -1));
			if (R_FAILED(res)) goto cleanup;
		} else {
			// re-set all the new flags
			u32 ns_data_id_list[100];
			u16 entries_read;
			res = bossGetNsDataIdList(0xFFFFFFFF, 100, 0, 0, ns_data_id_list, &entries_read, NULL);
			if (R_SUCCEEDED(res)) {
				for (int i = 0; i < entries_read; i++) {
					logln(INFO, "Found NS Data id %lx", ns_data_id_list[i]);
					_e(bossSetNsDataNewFlag(ns_data_id_list[i], false));
				}
			}
			bossSetAppNewFlag(FULL_NETPASS_ID, false);
		}
	}
	
	// ok, storage is set up. Now, set up the boss tasks
	
	ctx = malloc(sizeof(BossContext));
	if (!ctx) {
		res = _e(ERROR_OUT_OF_MEMORY);
		goto cleanup;
	}

	char lang[6] = "en\0\0\0\0";
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (get_language() == all_languages[i]) {
			strncpy(lang, all_languages_str[i], 5);
			lang[0] = tolower(lang[0]);
			lang[1] = tolower(lang[1]);
			break;
		}
	}
	char url[100];
	
	char* update_task = is_3dsx ? "upd3dsx" : "updcia";
	
	// news task
	for (int i = 0; i < NUM_NEWS_TASKS; i++) {
		snprintf(url, 100, "https://api.netpass.cafe/npdl/p01/nsa/netpass/news/%s/NEWS%d", lang, i);
		bossSetupContext(ctx, 60*60*6, url);
		ctx->priority = 0x7D; // re-set priority
		snprintf(url, 100, "news%d", i);
		res = _e(upsertSpotpassTask(url, ctx));
		if (R_FAILED(res)) goto cleanup;
	}

	// update task
	snprintf(url, 100, "https://api.netpass.cafe/npdl/p01/nsa/netpass/%s/%s/netpass.%s", update_task, lang, is_3dsx ? "3dsx" : "cia");
	bossSetupContext(ctx, 60*60*24, url);
	ctx->priority = 0x7D; // re-set priority
	strncpy(ctx->httpHeaders[0].name, "3ds-netpass-version", 0x20);
#ifdef _VERSION_GIT_SHA_
	// cppcheck-suppress invalidPrintfArgType_s
	snprintf(url, sizeof(url),
			 "v%d.%d.%d+%s",
			 _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_, _VERSION_GIT_SHA_);
#else
	snprintf(url, sizeof(url),
			 "v%d.%d.%d",
			 _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
#endif
	strncpy(ctx->httpHeaders[0].value, url, 0x100);
	res = _e(upsertSpotpassTask(update_task, ctx));
	if (R_FAILED(res)) goto cleanup;

	_e(setupSharedIconCache(FULL_NETPASS_ID, smdh));
cleanup:
	if (smdh) free(smdh);
	if (ctx) free(ctx);
	return res; //return _e(setupNotificationIcon());
}
