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

#include "spotpass.h"
#include "boss.h"
#include "strings.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMDH_SIZE 14016
#define NUM_NEWS_TASKS 5

Result taskDiffers(const char* task_id, bossContext* ctx, bool* result) {
	*result = true;

	// fist load the correct task
	Result res = bossGetTaskInfo(task_id, 0);
	if (R_FAILED(res)) return 0; // task does not exist
	
	// compare interval
	u32 interval;
	res = bossGetTaskInterval(task_id, &interval);
	if (R_FAILED(res) || interval != ctx->property[0x3]) return res;

	// compare priority
	u8 priority;
	res = bossGetTaskPriority(task_id, &priority);
	if (R_FAILED(res) || priority != ctx->property[0x0]) return res;

	char* buf = malloc(0x360);
	if (!buf) return ERROR_OUT_OF_MEMORY;
	memset(buf, 0, 0x360);
	
	// compare url
	res = bossReceiveProperty(0x7, buf, 0x200);
	if (R_FAILED(res) || strncmp(buf, ctx->url, 0x200) != 0) {
		free(buf);
		return res;
	}

	// compare headers
	res = bossReceiveProperty(0xD, buf, 0x360);
	if (R_FAILED(res)) {
		free(buf);
		return res;
	}
	for (int i = 0; i < 3; i++) {
		int offset = i*0x120;
		if (strncmp(buf + offset, ctx->property_xd + offset, 0x20) != 0 || strncmp(buf + offset + 0x20, ctx->property_xd + offset + 0x20, 0x100) != 0) {
			free(buf);
			return res;
		}
	}
	free(buf);

	// check if count is too small
	u32 count;
	res = bossReceiveProperty(0x4, &count, 4);
	if (R_FAILED(res) || (count < 10 && ctx->property[0x4] > 10)) return res;
	// ok, all we care about is the same
	*result = false;
	return res;
}

Result upsertTask(const char* task_id, bossContext* ctx) {
	bool differs;
	Result res = taskDiffers(task_id, ctx, &differs);
	if (R_FAILED(res) || !differs) return res;
	logln(INFO, "Updating BOSS task: %s", task_id);
	bossDeleteTask(task_id, 0);
	res = bossSendContextConfig(ctx);
	if (R_FAILED(res)) return res;
	res = bossRegisterTask(task_id, 0, 0);
	if (R_FAILED(res)) return res;
	return bossStartTask(task_id);
}

Result setupSpotpass(bool is_3dsx) {
	// first init boss with our id
	// As we may be running via the homebrew menu we need our own id
	// and priv mode so that we can pretend we are always running as cia.
	// Because of homebrew we can do this! :D
	const u32 netpass_id = 0xF6574;

	//cecdOpenRawFile(netpass_lower, CEC_PATH_MBOX_DIR, 8, NULL);
	
	Result res = _e(bossInit(0x0004000000000000ull | (netpass_id << 8), false));
	if (R_FAILED(res)) return res;

	// Now, setting up storage.
	{
		u8* smdh = malloc(SMDH_SIZE);
		if (!smdh) return _e(ERROR_OUT_OF_MEMORY);
		// first, we check if we need to set it up at all
		
		FS_ExtSaveDataInfo info = {
			mediaType: MEDIATYPE_SD,
			saveId: netpass_id,
		};

		if (R_FAILED(FSUSER_ReadExtSaveDataIcon(NULL, info, SMDH_SIZE, smdh))) {
			// Ok, we have to set it up. So, for that we need to read our smdh.
			FILE* f = fopen("romfs:/netpass.smdh", "rb");
			if (!f) {
				res = _e_errno();
				free(smdh);
				return res;
			}
			if (fread(smdh, SMDH_SIZE, 1, f) != 1) {
				res = _e_errno();
				fclose(f);
				free(smdh);
				return res;
			} 
			fclose(f);
	
			res = _e(FSUSER_CreateExtSaveData(info, 42, 42, -1, SMDH_SIZE, (u8*)smdh));
			free(smdh);
			if (R_FAILED(res)) return res;
	
			res = _e(bossSetStorageInfo(netpass_id, -1, MEDIATYPE_SD));
			if (R_FAILED(res)) return res;
		} else {
			free(smdh);
		}
	}
	
	// ok, storage is set up. Now, set up the boss tasks
	
	bossSetOptoutFlag(false);
	bossSetAppNewFlag(0x0004000000000000ull | (netpass_id << 8), false);
	bossSetNsDataNewFlag(0x1, false);
	bossSetNsDataNewFlag(0x2, false);
	
	bossContext* ctx = malloc(sizeof(bossContext));
	if (!ctx) return _e(ERROR_OUT_OF_MEMORY);

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
		bossSetupContextDefault(ctx, 60*60*24, url);
		ctx->property[0x0] = 0x7D; // re-set priority
		snprintf(url, 100, "news%d", i);
		res = _e(upsertTask(url, ctx));
		if (R_FAILED(res)) return res;
	}

	// update task
	snprintf(url, 100, "https://api.netpass.cafe/npdl/p01/nsa/netpass/%s/%s/netpass.%s", update_task, lang, is_3dsx ? "3dsx" : "cia");
	bossSetupContextDefault(ctx, 60, url);
	ctx->property[0x0] = 0x7D; // re-set priority
	strncpy(ctx->property_xd, "3ds-netpass-version", 0x20);
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
	strncpy(ctx->property_xd + 0x20, url, 0x100);
	res = _e(upsertTask(update_task, ctx));
	if (R_FAILED(res)) return res;
	
	return res;
}
