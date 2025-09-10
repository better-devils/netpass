/**
 * NetPass
 * Copyright (C) 2024-2025 Sorunome
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
#include "cecd.h"

//#define BASE_URL "https://api.netpass.cafe"
#define BASE_URL "https://devapi.netpass.cafe"

#define RULES_URL "http://netpass.cafe/rules.html"
#define PRIVACY_URL "http://netpass.cafe/privacy.html"

#ifndef __GNUC__
// workaround only for non-gnuc diagnostics
#define lambda(return_type, function_body) ((void*) 0)
#else
#define lambda(return_type, function_body) \
({ \
	return_type __fn__ function_body \
		__fn__; \
})
#endif

typedef struct {
	u32 title_id;
	char name[65];
} TitleDataEntry;

typedef struct {
	int num_titles;
	TitleDataEntry titles[12];
} NetpassTitleData;

typedef struct {
	bool new_version_available;
	u8 major;
	u8 minor;
	u8 patch;
} PingResponseVersion;

typedef struct {
	bool is_banned;
	CecTimestamp time_start;
	CecTimestamp time_end;
	char* reason;
} PingResponseBan;

typedef struct {
	char* message;
} PingResponseMessage;

typedef struct {
	PingResponseVersion version;
	PingResponseBan ban;
	PingResponseMessage message;
} PingResponse;

typedef struct {
	s32 id;
	u8 uuid[16];
	bool have_image;
	u8 image_hash[0x20];
	CecTimestamp time_start;
	CecTimestamp time_end;
	u32 time_remaining;
	char name[100];
	char artist_name[100];
} LocationResponse;

Result readPingResponse(PingResponse* response, u8* buf, u32 len);
Result initTitleData(void);
NetpassTitleData* getTitleData(void);
int numUsedTitles(void);
void clearIgnoredTitles(CecMboxListHeader* mbox_list);

Result doSlotExchangeRetry(void);
Result getLocation(void);
Result setLocation(int location);
Result setEventLocation(u8 uuid[16]);

void bgLoopInit(void);
void bgLoopExit(void);
void triggerDownloadInboxes(void);

s32 main_thread_prio(void);
void init_main_thread_prio(void);

extern LocationResponse location;
extern FS_Archive sharedextdata_b;
