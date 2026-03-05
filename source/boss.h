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

Result bossGetStorageInfo(u64* exdata_id, u32* boss_size, u8* extdata_type);
Result bossUnregisterTask(char* task_id, u16 step_id);
Result bossReconfigureTask(char* task_id, u16 step_id);
Result bossGetTaskIdList(void);
Result bossReceiveProperty(BossPropertyId propertyId, void* buf, u32 size);
Result bossStartTask(char* task_id);
Result bossCancelTask(char* task_id);
