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
#define CUSTOM_IMS_HEADER "3ds-if-modified-since"

enum TaskDiffersResult {
	TaskSame,
	TaskReconfigure,
	TaskRedo,
	TaskCreate,
};

Result taskDiffers(const char* task_id, bossContext* ctx, enum TaskDiffersResult* result) {
	// re-doing a task is a more bulletproof (but less nice) thing than
	// re-configuring it.
	// So, to determine what to do, we first have to test for all the things that
	// will require re-doing it completely.
	*result = TaskCreate;

	// fist load the correct task
	Result res = bossGetTaskInfo(task_id, 0);
	if (R_FAILED(res)) return 0; // task does not exist

	*result = TaskRedo;

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
		if (
			strncmp(buf + offset, ctx->property_xd + offset, 0x20) != 0
			|| strncmp(buf + offset + 0x20, ctx->property_xd + offset + 0x20, 0x100) != 0
		) {
			if (strncmp(buf + offset, CUSTOM_IMS_HEADER, 0x20) == 0) continue;
			free(buf);
			return res;
		}
	}
	free(buf);
	
	*result = TaskReconfigure;
	
	// compare interval
	u32 interval;
	res = bossGetTaskInterval(task_id, &interval);
	if (R_FAILED(res) || interval != ctx->property[0x3]) return res;

	// compare priority
	u8 priority;
	res = bossGetTaskPriority(task_id, &priority);
	if (R_FAILED(res) || priority != ctx->property[0x0]) return res;

	// check if count is too small
	u32 count;
	res = bossReceiveProperty(0x4, &count, 4);
	if (R_FAILED(res) || (count < 10 && ctx->property[0x4] > 10)) return res;

	// ok, all we care about is the same
	*result = TaskSame;
	return res;
}

Result upsertTask(const char* task_id, bossContext* ctx) {
	enum TaskDiffersResult differs;
	Result res = taskDiffers(task_id, ctx, &differs);
	if (R_FAILED(res) || differs == TaskSame) return res;
	if (differs == TaskReconfigure) {
		logln(INFO, "Reconfiguring BOSS task: %s", task_id);
		res = bossSendContextConfig(ctx);
		if (R_FAILED(res)) return res;
		return bossReconfigureTask(task_id, 0);
		
	}
	logln(INFO, "Redoing BOSS task: %s", task_id);
	if (differs != TaskCreate) {
		// read the old if-modified-since header and copy it to a free header slot
		res = bossGetTaskStatus(task_id, 0);
		if (R_FAILED(res)) return res;
		char* ims = malloc(0x40);
		if (!ims) return ERROR_OUT_OF_MEMORY;
		res = bossReceiveProperty(0x2F, ims, 0x40);
		if (R_FAILED(res)) {
			free(ims);
			return res;
		}
		
		if (*ims) {
			for (int i = 0; i < 3; i++) {
				int offset = i*0x120;
				if (*(ctx->property_xd + offset)) continue;
				strncpy(ctx->property_xd + offset, CUSTOM_IMS_HEADER, 0x20);
				strncpy(ctx->property_xd + offset + 0x20, ims, 0x40);
				break;
			}
		}
		bossDeleteTask(task_id, 0);
		free(ims);
	}
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
	const u64 full_netpass_id = 0x0004000000000000ull | (netpass_id << 8);

	//cecdOpenRawFile(netpass_lower, CEC_PATH_MBOX_DIR, 8, NULL);
	
	Result res = _e(bossInit(full_netpass_id, false));
	if (R_FAILED(res)) return res;

	// Now, setting up storage.
	{
		// first, we check if we need to set it up at all
		
		FS_ExtSaveDataInfo info = {
			mediaType: MEDIATYPE_SD,
			saveId: netpass_id,
		};
		if (R_FAILED(bossGetStorageInfo(NULL)) || R_FAILED(FSUSER_ReadExtSaveDataIcon(NULL, info, 0, NULL))) {
			u8* smdh = malloc(SMDH_SIZE);
			if (!smdh) return _e(ERROR_OUT_OF_MEMORY);

			// Ok, we have to set it up. So, for that we need to read our smdh.
			logln(INFO, "Setting up spotpass storage...");
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

			//res = _e(bossRegisterStorageEntry(netpass_id, -1, 0, MEDIATYPE_SD));
			//if (R_FAILED(res)) return res;
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
			bossSetAppNewFlag(full_netpass_id, false);
		}
	}
	
	// ok, storage is set up. Now, set up the boss tasks

	// TODO: properly handle optout flag
	_e(bossSetOptoutFlag(false));
	
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
		bossSetupContextDefault(ctx, 60*60*6, url);
		ctx->property[0x0] = 0x7D; // re-set priority
		snprintf(url, 100, "news%d", i);
		res = _e(upsertTask(url, ctx));
		if (R_FAILED(res)) return res;
	}

	// update task
	snprintf(url, 100, "https://api.netpass.cafe/npdl/p01/nsa/netpass/%s/%s/netpass.%s", update_task, lang, is_3dsx ? "3dsx" : "cia");
	bossSetupContextDefault(ctx, 60*60*24, url);
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
