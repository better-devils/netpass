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

Result bossGetStorageInfo(u32* storage_size) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x4, 0, 0);

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;

	if (storage_size) *storage_size = cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result bossSetOptoutFlag(bool flag) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x9, 1, 0);
	cmdbuf[1] = flag;

	if(R_FAILED(res = svcSendSyncRequest(bossGetSessionHandle()))) return res;
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

Result bossGetStepIdList(const char* task_id) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0xF, 1, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossGetNsDataIdList(u32 filter, u32 max_entries, u16 start_index, u32 start_data_id, u32* entries, u16* num_entries, u16* end_index) {
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x10, 4, 2);
	cmdbuf[1] = filter;
	cmdbuf[2] = max_entries;
	cmdbuf[3] = start_index;
	cmdbuf[4] = start_data_id;

	cmdbuf[5] = IPC_Desc_Buffer(max_entries * sizeof(u32), IPC_BUFFER_W);
	cmdbuf[6] = (u32)entries;
	
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	if (num_entries) *num_entries = cmdbuf[2];
	if (end_index) *end_index = cmdbuf[3];
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

Result bossGetTaskInterval(const char* task_id, u32* interval) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x19, 1, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	ret = (Result)cmdbuf[1];
	if (R_SUCCEEDED(ret) && interval) *interval = cmdbuf[2];
	return ret;
}

Result bossGetTaskCount(const char* task_id, u32* count) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x1A, 1, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	ret = (Result)cmdbuf[1];
	if (R_SUCCEEDED(ret) && count) *count = cmdbuf[2];
	return ret;
}

Result bossGetTaskServiceStatus(const char* task_id, u8* service_status) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x1B, 1, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	ret = (Result)cmdbuf[1];
	if (R_SUCCEEDED(ret) && service_status) *service_status = (u8)cmdbuf[2];
	return ret;
}

Result bossStartTask(const char* task_id) {
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

Result bossCancelTask(const char* task_id) {
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

Result bossGetTaskCommErrorCode(const char* task_id, u32* err_code, u32* count, u8* current_step) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x22, 1, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	ret = (Result)cmdbuf[1];
	if (R_SUCCEEDED(ret)) {
		if (err_code) *err_code = cmdbuf[2];
		if (count) *count = cmdbuf[3];
		if (current_step) *current_step = (u8)cmdbuf[4];
	}
	return ret;
}

Result bossGetTaskStatus(const char* task_id, u8 step_id) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x23, 3, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = true;
	cmdbuf[3] = step_id;
	cmdbuf[4] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[5] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossGetTaskError(const char* task_id, u8 step_id) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x24, 2, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = step_id;
	cmdbuf[3] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[4] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossGetTaskInfo(const char* task_id, u8 step_id) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x25, 2, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = step_id;
	cmdbuf[3] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[4] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossSetNsDataNewFlag(u32 ns_data_id, bool new) {
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x2B, 2, 0);
	cmdbuf[1] = ns_data_id;
	cmdbuf[2] = new;

	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossRegisterStorageEntry(u64 title_id, u32 storage_size, u16 entry_id, u8 media_type) {
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x2F, 5, 0);
	cmdbuf[1] = (u32) title_id;
	cmdbuf[2] = (u32) (title_id >> 32);
	cmdbuf[3] = storage_size;
	cmdbuf[4] = entry_id;
	cmdbuf[5] = media_type;
	
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossGetTaskPriority(const char* task_id, u8* priority) {
	u32 size = strlen(task_id) + 1; // include 0 pointer
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x34, 1, 2);
	cmdbuf[1] = size;
	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)task_id;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	ret = (Result)cmdbuf[1];
	if (R_SUCCEEDED(ret) && priority) *priority = (u8)cmdbuf[2];
	return ret;
}

Result bossSetAppNewFlag(u64 app_id, bool flag) {
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x404, 1, 0);
	cmdbuf[1] = flag;
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}

Result bossGetAppIdList(void) {
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40A, 0, 0);
	if (R_FAILED(ret = svcSendSyncRequest(bossGetSessionHandle()))) return ret;
	return (Result)cmdbuf[1];
}
