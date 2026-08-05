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

typedef struct BossHTTPHeader {
	char name[0x20];
	char value[0x100];
} BossHTTPHeader;

typedef enum {
	TITLE_SWAPDOODLE     = 0x001a2c00,
} BossTitles;

typedef enum BossPropertyId {
	BOSSPROPERTY_DURATION = 0x04,
	BOSSPROPERTY_URL = 0x07,
	BOSSPROPERTY_HTTPHEADERS = 0x0D,
	BOSSPROPERTY_TOTALTASKS = 0x35,
	BOSSPROPERTY_TASKIDS = 0x36,
} BossPropertyId;

typedef BossHTTPHeader* BossHTTPHeaders;

Result bossGetStorageInfo(u32* storage_size);
Result bossSetOptoutFlag(bool flag);
Result bossGetOptoutFlag(bool* flag);
Result bossUnregisterTask(char* task_id, u16 step_id);
Result bossReconfigureTask(const char* task_id, u16 step_id);
Result bossGetTaskIdList(void);
Result bossGetStepIdList(const char* task_id);
Result bossGetNsDataIdList(u32 filter, u32 max_entries, u16 start_index, u32 start_data_id, u32* entries, u16* num_entries, u16* end_index);
Result bossReceiveProperty(BossPropertyId propertyId, void* buf, u32 size);
Result bossGetTaskInterval(const char* task_id, u32* interval);
Result bossGetTaskCount(const char* task_id, u32* count);
Result bossGetTaskServiceStatus(const char* task_id, u8* service_status);
Result bossStartTask(const char* task_id);
Result bossCancelTask(const char* task_id);
Result bossGetTaskCommErrorCode(const char* task_id, u32* err_code, u32* count, u8* current_step);
Result bossGetTaskStatus(const char* task_id, u8 step_id);
Result bossGetTaskError(const char* task_id, u8 step_id);
Result bossGetTaskInfo(const char* task_id, u8 step_id);
Result bossSetNsDataNewFlag(u32 ns_data_id, bool new);
Result bossRegisterStorageEntry(u64 title_id, u32 storage_size, u16 entry_id, u8 media_type);
Result bossGetTaskPriority(const char* task_id, u8* priority);
Result bossSetAppNewFlag(u64 app_id, bool flag);
Result bossGetAppIdList(void);
