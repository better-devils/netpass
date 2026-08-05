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
#include "api.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_NEWS_TASKS 5
#define CUSTOM_IMS_HEADER "3ds-if-modified-since"
#define NETPASS_ID (0xF6574)
#define FULL_NETPASS_ID ((u64)0x0004000000000000ull | (NETPASS_ID << 8))

typedef struct SmdhHeader {
	u32 magic;
	u16 version;
	u16 reserved;
} SmdhHeader;

typedef struct SmdhTitle {
	u16 name[0x40];
	u16 desc[0x80];
	u16 publisher[0x40];
} SmdhTitle;

typedef struct SmdhSettings {
	u8 game_ratings[0x10];
	u32 region_lock;
	u8 match_maker_id[0xC];
	u32 flags;
	u16 eula_version;
	u16 reserved;
	u32 default_frame;
	u32 cec_id;
} SmdhSettings;

typedef struct Smdh {
	SmdhHeader header;
	SmdhTitle title[0x10];
	SmdhSettings settings;
	u8 reserved[0x8];
	u8 small_icon[0x480];
	u16 big_icon[0x900];
} Smdh;

typedef struct SharedSmdh {
	u8 unc[0x20];
	SmdhTitle title[0x10];
	u16 small_icon[24*24];
	u16 big_icon[48*48];
} SharedSmdh;

typedef struct {
	u64 unc;
	u64 title_id;
} SharedTitleEntry;

static Result setupNotificationIcon(void) {
	// As our app is hidden from activity the icon won't be in the shared icon cache
	// so...we have to add it manually
	Result res = 0;
	FILE* idb = NULL;
	FILE* idbt = NULL;
	SharedTitleEntry* shared_titles = NULL;
	SharedSmdh* shared_icons = NULL;
	Smdh* smdh = NULL;
	FILE* f_smdh = NULL;

	// first we determine the offset
	idbt = fopen("sharedextdata_b:/idbt.dat", "rb");
	if (!idbt) goto fail_errno;
	fseek(idbt, 0, SEEK_END);
	u64 filesize = ftell(idbt);
	fseek(idbt, 0, SEEK_SET);
	int num_shared_titles = filesize / sizeof(SharedTitleEntry);
	shared_titles = malloc(filesize);
	if (!shared_titles) {
		res = ERROR_OUT_OF_MEMORY;
		goto exit;
	}
	if (fread(shared_titles, filesize, 1, idbt) != 1) goto fail_errno;
	fclose(idbt);
	idbt = NULL;
	int found_offset = -1;
	int free_offset = -1;
	for (int i = 0; i < num_shared_titles; i++) {
		if (shared_titles[i].title_id == FULL_NETPASS_ID) {
			found_offset = i;
			break;
		} else if (free_offset == -1 && (!shared_titles[i].title_id || shared_titles[i].title_id == 0xFFFFFFFFFFFFFFFFull)) {
			free_offset = i;
		}
	}
	// if our application is already in the cache, then nothing to do
	if (found_offset != -1) goto exit;
	// if there is also no free slot...error out
	if (free_offset == -1) {
		res = -1;
		goto exit;
	}
	// Create the icon in the shared icon cache metadata first
	logln(INFO, "Icon not in shared icon cache, creating it...");
	found_offset = free_offset;
	shared_titles[found_offset].unc = 0;
	shared_titles[found_offset].title_id = FULL_NETPASS_ID;
	Handle handle;
	res = FSUSER_OpenFile(&handle, sharedextdata_b, fsMakePath(PATH_ASCII, "/idbt.dat"), FS_OPEN_WRITE, 0);
	if (R_FAILED(res)) goto exit;
	res = FSFILE_Write(handle, NULL, 0, shared_titles, filesize, FS_WRITE_FLUSH);
	FSFILE_Close(handle);
	if (R_FAILED(res)) goto exit;
	free(shared_titles);
	shared_titles = 0;

	// Now read our own smdh
	f_smdh = fopen("romfs:/netpass.smdh", "rb");
	if (!f_smdh) goto fail_errno;
	smdh = malloc(sizeof(Smdh));
	if (!smdh) {
		res = ERROR_OUT_OF_MEMORY;
		goto exit;
	}
	if (fread(smdh, sizeof(Smdh), 1, f_smdh) != 1) goto fail_errno;
	fclose(f_smdh);
	f_smdh = NULL;

	// Open the icon database
	idb = fopen("sharedextdata_b:/idb.dat", "rb");
	if (!idb) goto fail_errno;
	fseek(idb, 0, SEEK_END);
	filesize = ftell(idb);
	fseek(idb, 0, SEEK_SET);
	num_shared_titles = filesize / sizeof(SharedSmdh);
	// do some sanity checks
	if (num_shared_titles <= found_offset) {
		res = -2;
		goto exit;
	}
	shared_icons = malloc(filesize);
	if (!shared_icons) goto fail_errno;
	if (fread(shared_icons, filesize, 1, idb) != 1) goto fail_errno;
	fclose(idb);
	idb = NULL;
	// ...and copy the icon over to it
	memset(shared_icons[found_offset].unc, 0, 0x20);
	memcpy(shared_icons[found_offset].title, smdh->title, sizeof(SmdhTitle) * 0x10);
	memcpy(shared_icons[found_offset].small_icon, smdh->small_icon, 24*24*2);
	memcpy(shared_icons[found_offset].big_icon, smdh->big_icon, 48*48*2);

	res = FSUSER_OpenFile(&handle, sharedextdata_b, fsMakePath(PATH_ASCII, "/idb.dat"), FS_OPEN_WRITE, 0);
	if (R_FAILED(res)) goto exit;
	res = FSFILE_Write(handle, NULL, 0, shared_icons, filesize, FS_WRITE_FLUSH);
	FSFILE_Close(handle);
	if (R_FAILED(res)) goto exit;

	goto exit;
fail_errno:
	res = _e_errno();
exit:
	if (shared_titles) free(shared_titles);
	if (shared_icons) free(shared_icons);
	if (smdh) free(smdh);
	if (idb) fclose(idb);
	if (idbt) fclose(idbt);
	if (f_smdh) fclose(f_smdh);
	return res;
}

enum TaskDiffersResult {
	TaskSame,
	TaskReconfigure,
	TaskRedo,
	TaskCreate,
};

static Result taskDiffers(const char* task_id, bossContext* ctx, enum TaskDiffersResult* result) {
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

static Result upsertTask(const char* task_id, bossContext* ctx) {
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
		if (!*ims) {
			// while we don't have an ims header set, mayhaps we had a
			// backup ims header set that we need to copy over
			char* headers = malloc(0x360);
			if (!headers) {
				free(ims);
				return ERROR_OUT_OF_MEMORY;
			}
			res = bossReceiveProperty(0xD, headers, 0x360);
			if (R_FAILED(res)) {
				free(ims);
				free(headers);
				return res;
			}
			for (int i = 0; i < 3; i++) {
				int offset = i*0x120;
				if (strncmp(headers + offset, CUSTOM_IMS_HEADER, 0x20) == 0) {
					strncpy(ims, headers + offset + 0x20, 0x40-1); // -1 due to 0-byte
					break;
				}
			}
			free(headers);
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
	
	Result res = _e(bossInit(FULL_NETPASS_ID, false));
	if (R_FAILED(res)) return res;

	// Now, setting up storage.
	{
		// first, we check if we need to set it up at all
		
		FS_ExtSaveDataInfo info = {
			mediaType: MEDIATYPE_SD,
			saveId: NETPASS_ID,
		};
		if (R_FAILED(bossGetStorageInfo(NULL)) || R_FAILED(FSUSER_ReadExtSaveDataIcon(NULL, info, 0, NULL))) {
			Smdh* smdh = malloc(sizeof(Smdh));
			if (!smdh) return _e(ERROR_OUT_OF_MEMORY);

			// Ok, we have to set it up. So, for that we need to read our smdh.
			logln(INFO, "Setting up spotpass storage...");
			FILE* f = fopen("romfs:/netpass.smdh", "rb");
			if (!f) {
				res = _e_errno();
				free(smdh);
				return res;
			}
			if (fread(smdh, sizeof(Smdh), 1, f) != 1) {
				res = _e_errno();
				fclose(f);
				free(smdh);
				return res;
			} 
			fclose(f);
	
			res = _e(FSUSER_CreateExtSaveData(info, 42, 42, -1, sizeof(Smdh), (u8*)smdh));
			free(smdh);
			if (R_FAILED(res)) return res;
	
			res = _e(bossSetStorageInfo(NETPASS_ID, -1, MEDIATYPE_SD));
			if (R_FAILED(res)) return res;

			//res = _e(bossRegisterStorageEntry(NETPASS_ID, -1, 0, MEDIATYPE_SD));
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
			bossSetAppNewFlag(FULL_NETPASS_ID, false);
		}
	}
	
	// ok, storage is set up. Now, set up the boss tasks
	
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
	
	return _e(setupNotificationIcon());
}
