/**
 * NetPass
 * Copyright (C) 2024 Sorunome
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

#include <3ds.h>
#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/srv.h>
#include <3ds/synchronization.h>
#include "boss.h"
#include <3ds/ipc.h>
#include <string.h>

Result bossGetStorageInfo(u64* exdata_id, u32* boss_size, u8* extdata_type) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x4, 0, 0);

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;

	memcpy(exdata_id, &cmdbuf[2], sizeof(u64));
	*boss_size = cmdbuf[4];
	*extdata_type = cmdbuf[5];

	return (Result)cmdbuf[1];
}

Result bossUnregisterTask(char* task_id, u16 step_id) {
	Result res = 0;
	u32 size = strlen(task_id)+1;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0xC,2,2);
	cmdbuf[1] = size;
	cmdbuf[2] = step_id;
	cmdbuf[3] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[4] = (u32)task_id;

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
	return (Result)cmdbuf[1];
}

Result bossReconfigureTask(char* task_id, u16 step_id) {
	Result res = 0;
	u32 size = strlen(task_id)+1;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0xD,2,2);
	cmdbuf[1] = size;
	cmdbuf[2] = step_id;
	cmdbuf[3] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[4] = (u32)task_id;

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
	return (Result)cmdbuf[1];
}

Result bossGetTaskIdList(void) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0xE,0,0);

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
	return (Result)cmdbuf[1];
}

Result bossReceiveProperty(BossPropertyId propertyId, void* buf, u32 size) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x16,2,2);
	cmdbuf[1] = (u32) propertyId;
	cmdbuf[2] = (u32) size;
	cmdbuf[3] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[4] = (u32) buf;

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
	return (Result)cmdbuf[1];
}

Result bossStartTask(char* task_id) {
	Result res = 0;
	u32 size = strlen(task_id)+1;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x1C,1,2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
	return (Result)cmdbuf[1];
}

Result bossCancelTask(char* task_id) {
	Result res = 0;
	u32 size = strlen(task_id)+1;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x1E,1,2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
	return (Result)cmdbuf[1];
}
