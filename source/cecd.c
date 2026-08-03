/**
 * NetPass
 * Copyright (C) 2024-2026 Sorunome
 *               2026 Silentium
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

#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/srv.h>
#include <3ds/synchronization.h>
#include "cecd.h"
#include "utils.h"
#include <3ds/ipc.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>

static Handle cecdHandle;
static int cecdRefCount;
static bool in_spr_mode = false;

static void waitForNoSpr(void) {
	while (in_spr_mode) svcSleepThread((u64)1000000 * 100);
}

Result waitForCecdState(bool start, int command, CecStateAbbrev state) {
	Handle state_change_handle;
	Result res = 0;
	res = cecdGetChangeStateEventHandle(&state_change_handle);
	if (R_FAILED(res)) return res;
	res = start ? cecdStart(command) : cecdStop(command);
	if (R_FAILED(res)) return res;
	int count = 0;
	while (true) {
		count++;
		if (count > 20) {
			return ERROR_BAD_CECD_STATE;
		}
		svcWaitSynchronization(state_change_handle, 10e9);
		CecStateAbbrev is_state;
		res = cecdGetCecdState(&is_state);
		if (R_SUCCEEDED(res) && is_state == state) break;
	}
	svcCloseHandle(state_change_handle);
	in_spr_mode = command == CEC_COMMAND_OVER_BOSS || command == CEC_COMMAND_OVER_BOSS_FORCE || command == CEC_COMMAND_OVER_BOSS_FORCE_WAIT;
	return res;
}

static void getCurrentTime(CecTimestamp* cts) {
	time_t unix_time = time(NULL);
	struct tm* ts = gmtime((const time_t*)&unix_time);
	cts->hour = ts->tm_hour;
	cts->minute = ts->tm_min;
	cts->second = ts->tm_sec;
	cts->millisecond = 0;
	cts->year = ts->tm_year + 1900;
	cts->month = ts->tm_mon + 1;
	cts->day = ts->tm_mday;
	cts->weekday = ts->tm_wday;
}

Result cecdInit(void) {
	Result res = 0;

	if (AtomicPostIncrement(&cecdRefCount)) return 0;

	res = srvGetServiceHandle(&cecdHandle, "cecd:s");
	if (R_FAILED(res)) goto cleanup;

	return res;
cleanup:
	AtomicDecrement(&cecdRefCount);
	return res;
}

Result cecdGetState(u32* state) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x0E, 0, 0);
	
	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	*state = cmdbuf[2];

	return res;
}

Result cecdOpenRawFile(u32 program_id, u32 path_type, u32 open_flag, u32* out_filesize) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x01, 3, 2);
	cmdbuf[1] = program_id;
	cmdbuf[2] = path_type;
	cmdbuf[3] = open_flag;
	
	cmdbuf[4] = IPC_Desc_CurProcessId();
	cmdbuf[5] = 0;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];
	
	if (out_filesize) *out_filesize = cmdbuf[2];

	return res;
}

Result cecdReadRawFile(u32 size, u8* buf) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x02, 1, 2);
	cmdbuf[1] = size;

	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[3] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdReadMessage(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x03, 4, 4);
	cmdbuf[1] = program_id;
	cmdbuf[2] = (u32)is_outbox;
	cmdbuf[3] = 8; // message id size
	cmdbuf[4] = size;

	cmdbuf[5] = IPC_Desc_Buffer(8, IPC_BUFFER_R);
	cmdbuf[6] = (u32)message_id;
	cmdbuf[7] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[8] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdReadMessageWithHMAC(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id, u8* hmac) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x03, 4, 4);
	cmdbuf[1] = program_id;
	cmdbuf[2] = (u32)is_outbox;
	cmdbuf[3] = 8; // message id size
	cmdbuf[4] = size;

	cmdbuf[5] = IPC_Desc_Buffer(8, IPC_BUFFER_R);
	cmdbuf[6] = (u32)message_id;
	cmdbuf[7] = IPC_Desc_Buffer(32, IPC_BUFFER_R);
	cmdbuf[8] = (u32)hmac;
	cmdbuf[9] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[10] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdWriteRawFile(u32 size, u8* buf) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x05, 1, 2);
	cmdbuf[1] = size;

	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdWriteMessage(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x06, 4, 4);
	cmdbuf[1] = program_id;
	cmdbuf[2] = (u32)is_outbox;
	cmdbuf[3] = 8; // message id size
	cmdbuf[4] = size;

	cmdbuf[5] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[6] = (u32)buf;
	cmdbuf[7] = IPC_Desc_Buffer(8, IPC_BUFFER_RW);
	cmdbuf[8] = (u32)message_id;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdWriteMessageWithHMAC(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id, u8* hmac) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x07, 4, 6);
	cmdbuf[1] = program_id;
	cmdbuf[2] = (u32)is_outbox;
	cmdbuf[3] = 8; // message id size
	cmdbuf[4] = size;

	cmdbuf[5] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[6] = (u32)buf;
	cmdbuf[7] = IPC_Desc_Buffer(32, IPC_BUFFER_R);
	cmdbuf[8] = (u32)hmac;
	cmdbuf[9] = IPC_Desc_Buffer(8, IPC_BUFFER_RW);
	cmdbuf[10] = (u32)message_id;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdStart(CecCommand command) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x0B, 1, 0);
	cmdbuf[1] = command;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdStop(CecCommand command) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x0C, 1, 0);
	cmdbuf[1] = command;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdGetCecdState(CecStateAbbrev* state) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0xE, 0, 0);
	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];
	*state = cmdbuf[2];

	return res;
}

Result cecdGetCecInfoEventHandle(Handle* handle) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0xF, 0, 0);
	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];
	*handle = cmdbuf[3];

	return res;
}

Result cecdGetChangeStateEventHandle(Handle* handle) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x10, 0, 0);
	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];
	*handle = cmdbuf[3];

	return res;
}

Result cecdOpenAndWrite(u32 program_id, u32 path_type, u32 size, u8* buf) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x11, 4, 4);
	cmdbuf[1] = size;
	cmdbuf[2] = program_id;
	cmdbuf[3] = path_type;
	cmdbuf[4] = 0;

	cmdbuf[5] = IPC_Desc_CurProcessId();
	cmdbuf[6] = 0;
	cmdbuf[7] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[8] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdOpenAndRead(u32 program_id, u32 path_type, u32 size, u8* buf) {
	waitForNoSpr();
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x12, 4, 4);
	cmdbuf[1] = size;
	cmdbuf[2] = program_id;
	cmdbuf[3] = path_type;
	cmdbuf[4] = 0;

	cmdbuf[5] = IPC_Desc_CurProcessId();
	cmdbuf[6] = 0;
	cmdbuf[7] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[8] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprCreate(void) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40A, 0, 0);

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprInitialise(void) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40B, 0, 0);

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprGetSlotsMetadata(u32 size, SlotMetadata* buf, u32* slots_total) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40C, 1, 2);
	cmdbuf[1] = size;

	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[3] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];
	*slots_total = cmdbuf[2];

	return res;
}

Result cecdSprGetSlot(u32 title_id, u32 size, u8* buf) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40D, 2, 2);
	cmdbuf[1] = title_id;
	cmdbuf[2] = size;

	cmdbuf[3] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[4] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprSetTitleSent(u32 title_id, bool success) { // set send result
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40E, 2, 0);
	cmdbuf[1] = title_id;
	cmdbuf[2] = success;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprFinaliseSend(void) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x40F, 0, 0);

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprStartRecv(void) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x410, 0, 0);

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprAddSlotsMetadata(u32 size, u8* buf) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x411, 1, 2);
	cmdbuf[1] = size;

	cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[3] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprAddSlot(u32 title_id, u32 size, u8* buf) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x412, 3, 2);
	cmdbuf[1] = title_id;
	cmdbuf[2] = 0xFF; // flags
	cmdbuf[3] = size;

	cmdbuf[4] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[5] = (u32)buf;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprFinaliseRecv(void) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x413, 0, 0);

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdSprDone(bool success) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x414, 1, 0);
	cmdbuf[1] = success;

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result cecdGetBossUserid(u64* out) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x415, 0, 0);

	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];
	*out = (u64)cmdbuf[2] | ((u64)cmdbuf[3] << 32);

	return res;
}

// this method is broken
Result cecdGetSystemInfo(u32 destbuf_size, void* destbuf) {
	Result res = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	u8 parambuf[32];
	cmdbuf[0] = IPC_MakeHeader(0x0A, 3, 4); // 0x000A00C4
	cmdbuf[1] = 32; // dest buffer size
	cmdbuf[2] = 1; // info type
	cmdbuf[3] = 32; // param buffer size

	cmdbuf[4] = IPC_Desc_Buffer(32, IPC_BUFFER_R);
	cmdbuf[5] = (u32)parambuf;
	cmdbuf[4] = IPC_Desc_Buffer(destbuf_size, IPC_BUFFER_W);
	cmdbuf[5] = (u32)destbuf;

	
	if (R_FAILED(res = svcSendSyncRequest(cecdHandle))) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Handle cecdGetServHandle(void) {
	return cecdHandle;
}

Result updateStreetpassOutbox(u8* msgbuf) {
	Result res = 0;
	CecMessageHeader* msgheader = (CecMessageHeader*)msgbuf;
	// sanity checks
	if (msgheader->magic != 0x6060) return ERROR_INVALID_MESSAGE; // bad magic
	if (msgheader->message_size != msgheader->total_header_size + msgheader->body_size + 0x20) return ERROR_INVALID_MESSAGE;
	if (msgheader->message_size > MAX_MESSAGE_SIZE) return ERROR_INVALID_MESSAGE; // prooobably too large

	// first fetch how large the boxbuf is
	u8* boxbuf = malloc(sizeof(CecBoxInfoHeader));
	if (!boxbuf) {
		return _e(ERROR_OUT_OF_MEMORY);
	}
	res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_OUTBOX_INFO, sizeof(CecBoxInfoHeader), boxbuf));
	if (R_FAILED(res)) { // cecd fild not found
		goto cleanup_box;
	}
	// let's open the box buffer to update the metadata in there
	int max_boxbuf_size = sizeof(CecBoxInfoHeader) + sizeof(CecMessageHeader) * ((CecBoxInfoHeader*)boxbuf)->max_num_messages;
	free(boxbuf);
	boxbuf = malloc(max_boxbuf_size);
	if (!boxbuf) {
		return _e(ERROR_OUT_OF_MEMORY);
	}
	res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_OUTBOX_INFO, max_boxbuf_size, boxbuf));
	if (R_FAILED(res)) { // cecd fild not found
		goto cleanup_box;
	}
	CecBoxInfoHeader* boxheader = (CecBoxInfoHeader*)boxbuf;
	CecMessageHeader* boxmsgs = (CecMessageHeader*)(boxbuf + sizeof(CecBoxInfoHeader));

	int found_i = -1;
	for (int i = 0; i < boxheader->num_messages; i++) {
		if (0 == memcmp(boxmsgs[i].message_id, msgheader->message_id, sizeof(CecMessageId))){
			found_i = i;
			break;
		}
	}
	if (found_i < 0) {
		res = ERROR_DUPLICATE_MESSAGE; // not found
		goto cleanup_box;
	}
	memcpy(&boxmsgs[found_i], msgheader, sizeof(CecMessageHeader));

	res = _e(cecdOpenAndWrite(msgheader->title_id, CEC_PATH_OUTBOX_INFO, boxheader->file_size, boxbuf));
	if (R_FAILED(res)) { // cecd fild not found
		goto cleanup_box;
	}
	free(boxbuf);

	// now let's fetch the hmac and store the update
	CecMBoxInfoHeader mboxheader;
	res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_MBOX_INFO, sizeof(CecMBoxInfoHeader), (u8*)&mboxheader));
	if (R_FAILED(res)) return res;

	// great,we have all the bits we need now
	res = _e(cecdWriteMessageWithHMAC(
		msgheader->title_id, true,
		msgheader->message_size, msgbuf,
		msgheader->message_id, mboxheader.hmac_key));
	if (R_FAILED(res)) return res;

	return res;
cleanup_box:
	free(boxbuf);
	return res;
}

bool validateStreetpassMessage(u8* msgbuf) {
	CecMessageHeader* msgheader = (CecMessageHeader*)msgbuf;
	// sanity checks
	if (msgheader->magic != 0x6060) return false; // bad magic
	if (msgheader->message_size != msgheader->total_header_size + msgheader->body_size + 0x20) return false;
	if (msgheader->message_size > MAX_MESSAGE_SIZE) return false; // prooobably too large

	if (msgheader->body_size <= 0x20) {
		u8 b = 0;
		u8* ptr = msgbuf + msgheader->total_header_size;
		for (int i = 0; i < msgheader->body_size; i++) {
			b |= *ptr;
			ptr++;
		}
		if (b == 0) return false;
	}

	return true;
}

Result addStreetpassMessage(u8* msgbuf) {
	Result res = 0;
	CecMessageHeader* msgheader = (CecMessageHeader*)msgbuf;
	// sanity checks
	if (!validateStreetpassMessage(msgbuf)) return ERROR_INVALID_MESSAGE; // bad message

	// update the msg header about the receive time
	if (!msgheader->received.year) {
		getCurrentTime(&(msgheader->received));
	}

	// first fetch how large the boxbuf is
	u8* boxbuf = malloc(sizeof(CecBoxInfoHeader));
	if (!boxbuf) {
		return _e(ERROR_OUT_OF_MEMORY);
	}
	res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_INBOX_INFO, sizeof(CecBoxInfoHeader), boxbuf));
	if (R_FAILED(res)) {
		goto cleanup_box;
	}
	// let's open the box buffer to see if we can even accept a new message, or if the message has already been accepted
	int max_boxbuf_size = sizeof(CecBoxInfoHeader) + sizeof(CecMessageHeader) * ((CecBoxInfoHeader*)boxbuf)->max_num_messages;
	free(boxbuf);
	boxbuf = malloc(max_boxbuf_size);
	if (!boxbuf) {
		return _e(ERROR_OUT_OF_MEMORY);
	}
	res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_INBOX_INFO, max_boxbuf_size, boxbuf));
	if (R_FAILED(res)) { // cecd file not found
		goto cleanup_box;
	}
	CecBoxInfoHeader* boxheader = (CecBoxInfoHeader*)boxbuf;
	CecMessageHeader* boxmsgs = (CecMessageHeader*)(boxbuf + sizeof(CecBoxInfoHeader));

	for (int i = 0; i < boxheader->num_messages; i++) {
		if (0 == memcmp(boxmsgs[i].message_id, msgheader->message_id, sizeof(CecMessageId))){
			res = INFO_DUPLICATE_MESSAGE; // already added, nothing to do
			goto cleanup_box;
		}
	}
	if (boxheader->num_messages >= boxheader->max_num_messages) {
		res = _e(ERROR_BOX_FULL); // box already full
		goto cleanup_box;
	}

	// let's see if the message is too large for this box
	if (boxheader->max_message_size < msgheader->message_size) {
		res = _e(ERROR_TOO_LARGE);
		goto cleanup_box;
	}

	// now let's check if the box would overflow
	if (boxheader->box_size + msgheader->message_size > boxheader->max_box_size) {
		res = _e(ERROR_TOO_LARGE);
		goto cleanup_box;
	}

	// cool, the checks passed. Let's free the buffer for now, add our message, and then check if we gotta update the box
	free(boxbuf);

	{
		// box stuffs is done, let's fetch the mbox, to fetch the hmac key
		CecMBoxInfoHeader mboxheader;
		res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_MBOX_INFO, sizeof(CecMBoxInfoHeader), (u8*)&mboxheader));
		if (R_FAILED(res)) return res;

		msgheader->unopened = true;
		msgheader->new_flag = true;
		// great,we have all the bits we need now
		res = cecdWriteMessageWithHMAC(
			msgheader->title_id, false,
			msgheader->message_size, msgbuf,
			msgheader->message_id, mboxheader.hmac_key);
		if (R_FAILED(res)) return res;

		// check if the message was actually added...
		res = _e(cecdReadMessage(msgheader->title_id, false, 0, 0, msgheader->message_id));
		if (R_FAILED(res)) return res;
	}

	// let's see if we gotta update the metadata
	boxbuf = malloc(max_boxbuf_size);
	if (!boxbuf) {
		return _e(ERROR_OUT_OF_MEMORY);
	}
	res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_INBOX_INFO, max_boxbuf_size, boxbuf));
	if (R_FAILED(res)) { // cecd fild not found
		goto cleanup_box;
	}

	boxheader = (CecBoxInfoHeader*)boxbuf;
	boxmsgs = (CecMessageHeader*)(boxbuf + sizeof(CecBoxInfoHeader));
	bool found_box = false;
	for (int i = 0; i < boxheader->num_messages; i++) {
		if (0 == memcmp(boxmsgs[i].message_id, msgheader->message_id, sizeof(CecMessageId))){
			found_box = true;
			break;
		}
	}
	
	if (!found_box) {
		// ok, let's add the message to the box
		memcpy(boxbuf + sizeof(CecBoxInfoHeader) + sizeof(CecMessageHeader) * ((CecBoxInfoHeader*)boxbuf)->num_messages, msgbuf, sizeof(CecMessageHeader));
		boxheader->num_messages++;
		boxheader->file_size += sizeof(CecMessageHeader);
		boxheader->box_size += msgheader->message_size;
		res = _e(cecdOpenAndWrite(msgheader->title_id, CEC_PATH_INBOX_INFO, boxheader->file_size, boxbuf));
		if (R_FAILED(res)) { // cecd fild not found
			goto cleanup_box;
		}
		free(boxbuf);
	}


	{
		// now set the green notification dot
		// gotta re-fetch the mbox header, in case it changed
		CecMBoxInfoHeader mboxheader;
		res = _e(cecdOpenAndRead(msgheader->title_id, CEC_PATH_MBOX_INFO, sizeof(CecMBoxInfoHeader), (u8*)&mboxheader));
		if (R_FAILED(res)) return res;

		getCurrentTime(&(mboxheader.last_received));
		mboxheader.flag_unread = 1; // set the new notification dot
		mboxheader.flag_new = 1;

		res = _e(cecdOpenAndWrite(msgheader->title_id, CEC_PATH_MBOX_INFO, sizeof(CecMBoxInfoHeader), (u8*)&mboxheader));
		if (R_FAILED(res)) return res;
	}

	return res;
cleanup_box:
	free(boxbuf);
	return res;
}

Result registerStreetpassApplication(u32 title_id) {
	Result res;

	// first, we see if we are already added and if there is space for us
	{
		CecMboxListHeader mboxlist;
		res = _e(cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mboxlist), (u8*)&mboxlist));
		if (R_FAILED(res)) return res;

		for (int i = 0; i < mboxlist.num_boxes; i++) {
			u32 have_title_id = strtol((const char*)mboxlist.box_names[i], NULL, 16);
			if (have_title_id == title_id) return 0; // all good, already registered
		}

		if (mboxlist.num_boxes >= 12) return -1; // already full, sorry
	}

	// now, create the needed folders
	res = _e(cecdOpenRawFile(title_id, CEC_PATH_MBOX_DIR, 8, NULL));
	if (R_FAILED(res)) return res;
	res = _e(cecdOpenRawFile(title_id, CEC_PATH_INBOX_DIR, 8, NULL));
	if (R_FAILED(res)) return res;
	res = _e(cecdOpenRawFile(title_id, CEC_PATH_OUTBOX_DIR, 8, NULL));
	if (R_FAILED(res)) return res;

	// now, create the mbox
	{
		CecMBoxInfoHeader mbox = {
			magic: 0x6363,
			program_id: title_id,
			box_type_flags: 0x1,
			enabled: true,
		};
		// TODO: set an actual hmac key
		memset(mbox.hmac_key, 0, 32);
		memcpy(mbox.hmac_key, &title_id, 4); // lol
		res = _e(cecdOpenRawFile(title_id, CEC_PATH_MBOX_INFO, 6, NULL));
		if (R_FAILED(res)) return res;
		res = _e(cecdWriteRawFile(sizeof(CecMBoxInfoHeader), (u8*)&mbox));
		if (R_FAILED(res)) return res;
	}

	// now, create the inbox
	{
		CecBoxInfoHeader inbox = {
			magic: 0x6262,
			file_size: sizeof(CecBoxInfoHeader),
			max_box_size: 262144, // TODO: change
			box_size: 0,
			max_num_messages: 25, // TODO: change
			num_messages: 0,
			max_batch_size: 25, // TODO: change
			max_message_size: 102400, // TODO: change
		};
		//res = _e(cecdOpenAndWrite(title_id, CEC_PATH_INBOX_INFO, sizeof(CecBoxInfoHeader), (u8*)&inbox));
		//if (R_FAILED(res)) return res;
		res = _e(cecdOpenRawFile(title_id, CEC_PATH_INBOX_INFO, (1 << 4) | (1 << 2), NULL));
		if (R_FAILED(res)) return res;
		res = _e(cecdWriteRawFile(sizeof(CecBoxInfoHeader), (u8*)&inbox));
		if (R_FAILED(res)) return res;
	}

	// now, create the outbox
	{
		CecBoxInfoHeader outbox = {
			magic: 0x6262,
			file_size: sizeof(CecBoxInfoHeader),
			max_box_size: 102400, // TODO: change
			box_size: 0,
			max_num_messages: 1, // TODO: change
			num_messages: 0,
			max_batch_size: 1, // TODO: change
			max_message_size: 102400, // TODO: change
		};
		res = _e(cecdOpenRawFile(title_id, CEC_PATH_OUTBOX_INFO, 6, NULL));
		if (R_FAILED(res)) return res;
		res = _e(cecdWriteRawFile(sizeof(CecBoxInfoHeader), (u8*)&outbox));
		if (R_FAILED(res)) return res;

		CecOBIndex index = {
			magic: 0x6767,
			num_messages: 0,
		};
		res = _e(cecdOpenRawFile(title_id, CEC_PATH_OUTBOX_INDEX, 6, NULL));
		if (R_FAILED(res)) return res;
		res = _e(cecdWriteRawFile(sizeof(CecOBIndex), (u8*)&index));
		if (R_FAILED(res)) return res;
	}

	// create the name and icon
	{
		// TODO: actual name
		u8 name[] = {'N', 0, 'e', 0, 't', 0, 'P', 0, 'a', 0, 's', 0, 's', 0, 0, 0};
		res = _e(cecdOpenRawFile(title_id, CECMESSAGE_BOX_TITLE, 6, NULL));
		if (R_FAILED(res)) return res;
		res = _e(cecdWriteRawFile(sizeof(name), name));
		if (R_FAILED(res)) return res;

		u8* icon = malloc(48*48*2);
		if (!icon) return ERROR_OUT_OF_MEMORY;
		// TODO: actual icon
		memset(icon, 0, 48*48*2);
		res = _e(cecdOpenRawFile(title_id, CECMESSAGE_BOX_ICON, 6, NULL));
		if (R_FAILED(res)) {
			free(icon);
			return res;
		}
		res = _e(cecdWriteRawFile(48*48*2, icon));
		free(icon);
		if (R_FAILED(res)) return res;
	}

	// finally, update mboxlist
	{
		CecMboxListHeader mboxlist;
		res = _e(cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mboxlist), (u8*)&mboxlist));
		if (R_FAILED(res)) return res;
		snprintf((char*)mboxlist.box_names[mboxlist.num_boxes], 0x10, "%08lx", title_id);
		res = _e(cecdOpenAndWrite(title_id, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mboxlist));
		if (R_FAILED(res)) return res;
	}
	
	return 0;
}
