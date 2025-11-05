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
 *<
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "api.h"
#include "cecd.h"
#include "utils.h"
#include "config.h"
#include "report.h"
#include "qr.h"
#include "curl-handler.h"
#include "image_cache.h"
#include <stdlib.h>
#include <string.h>

#define R_IS_CEC_RESTART(res) (CTR_RESULT_GET_SUMMARY(res) == CTR_RESULT_SUMMARY_INVALID_STATE && CTR_RESULT_GET_MODULE(res) == CTR_RESULT_MODULE_CEC)

#define _e_cec(x) ({ \
	Result r = x; \
	R_FAILED(r) && R_IS_CEC_RESTART(r) ? r : _e(r); \
})

LocationResponse location = {0};
FS_Archive sharedextdata_b = 0;
NetpassTitleData title_data;

Result readPingResponse(PingResponse* resp, u8* buf, u32 len) {
	QrBuffer buffer;
	qr_buffer_new(&buffer, buf, len);
	// check version
	if (qr_read_u32(&buffer) != 1) return ERROR_INVALID_SERVER_RESPONSE;
	// read newest version
	resp->version.new_version_available = qr_read_bool(&buffer);
	resp->version.major = qr_read_u8(&buffer);
	resp->version.minor = qr_read_u8(&buffer);
	resp->version.patch = qr_read_u8(&buffer);
	// read banned stuffs
	resp->ban.is_banned = qr_read_bool(&buffer);
	qr_read_align(&buffer, 4);
	qr_read_object(&buffer, &resp->ban.time_start, sizeof(CecTimestamp));
	qr_read_object(&buffer, &resp->ban.time_end, sizeof(CecTimestamp));
	u32 banmsg_len = qr_peek_u32(&buffer);
	if (banmsg_len) {
		char* banmsg = malloc(banmsg_len);
		if (!banmsg) return ERROR_OUT_OF_MEMORY;
		qr_read_string(&buffer, banmsg, banmsg_len);
		resp->ban.reason = banmsg;
		qr_read_align(&buffer, 4);
	} else {
		resp->ban.reason = 0;
	}
	u32 message_len = qr_peek_u32(&buffer);
	if (message_len) {
		char* msg = malloc(message_len);
		if (!msg) {
			if (resp->ban.reason) free(resp->ban.reason);
			return ERROR_OUT_OF_MEMORY;
		}
		qr_read_string(&buffer, msg, message_len);
		resp->message.message = msg;
		qr_read_align(&buffer, 4);
	} else {
		resp->message.message = 0;
	}
	return 0;
}

Result initTitleData(void) {
	Result res = 0;
	CecMboxListHeader mbox_list;
	res = _e(cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mbox_list));
	if (R_FAILED(res)) return res;
	u16 title_name_utf16[65];
	for (int i = 0; i < mbox_list.num_boxes; i++) {
		u32 title_id = strtol((const char*)mbox_list.box_names[i], NULL, 16);

		memset(title_name_utf16, 0, sizeof(title_name_utf16));
		res = _e(cecdOpenAndRead(title_id, CECMESSAGE_BOX_TITLE, sizeof(title_name_utf16)-2, (u8*)title_name_utf16));
		if (R_FAILED(res)) return res;

		memset(title_data.titles[i].name, 0, sizeof(title_data.titles[i].name));
		// SAFETY: utf16_to_utf8 does not write a null terminator, so we memset above
		utf16_to_utf8((u8*)title_data.titles[i].name, title_name_utf16, sizeof(title_data.titles[i].name)-1);

		char* ptr = title_data.titles[i].name;
		while (*ptr) {
			if (*ptr == '\n') *ptr = ' ';
			ptr++;
		}
		title_data.titles[i].title_id = title_id;
	}
	title_data.num_titles = mbox_list.num_boxes;
	return res;
}

NetpassTitleData* getTitleData(void) {
	return &title_data;
}

int numUsedTitles(void) {
	int num = 0;
	for (int i = 0; i < title_data.num_titles; i++) {
		if (!isTitleIgnored(title_data.titles[i].title_id)) num++;
	}
	return num;
}

void clearIgnoredTitles(CecMboxListHeader* mbox_list) {
	size_t pos = 0;
	for (size_t i = 0; i < mbox_list->num_boxes; i++) {
		u32 title_id = strtol((const char*)mbox_list->box_names[i], NULL, 16);
		if (!isTitleIgnored(title_id)) {
			if (pos != i) memcpy(mbox_list->box_names[pos], mbox_list->box_names[i], 16);
			pos++;
		}
	}
	memset(mbox_list->box_names[pos], 0, 16 * (mbox_list->num_boxes - pos));
	mbox_list->num_boxes = pos;
}


typedef struct SlotInfo {
	SlotMetadata metadata[12];
	void* slots[12];
} SlotInfo;

typedef struct TitleExtraInfo {
	u32 title_id;
} TitleExtraInfo;

Result uploadSlot(SlotMetadata* metadata) {
	Result res = 0;
	char url[50];
	if (!metadata->title_id) {
		return ERROR_NO_TITLE_ID; // something went wrong
	}
	if (metadata->size == 0 || metadata->send_method == 1) {
		// recv only, delete outbox
		snprintf(url, 50, "%s/outbox/%08lx", BASE_URL, metadata->title_id);
		res = _e(httpRequest("DELETE", url, 0, 0, 0, 0));
		return res;
	}

	// read extra metadata to send
	u8* slot = malloc(metadata->size);
	if (!slot) {
		return ERROR_OUT_OF_MEMORY;
	}

	// now it is time to *actually* fetch the slot
	res = _e(cecdSprGetSlot(metadata->title_id, metadata->size, slot));
	if (R_FAILED(res)) {
		free(slot);
		return res;
	}

	// now upload the slot
	snprintf(url, 50, "%s/outbox/slot", BASE_URL);
	res = _e(httpRequest("POST", url, metadata->size, slot, 0, 0));
	free(slot);
	return res;
}

Result downloadSlot(int i, SlotInfo* slotinfo) {
	Result res = 0;
	SlotMetadata* metadata = &slotinfo->metadata[i];
	if (metadata->send_method == 2) {
		// send-only, nothing to do
		metadata->size = 0;
		return res;
	}
	char url[100];
	snprintf(url, 100, "%s/inbox/%lx/slot", BASE_URL, metadata->title_id);
	CurlReply* reply;
	res = _e(httpRequest("GET", url, 0, 0, &reply, 0));
	if (R_FAILED(res)) goto fail;
	u32 http_code = res;
	if (http_code == 204) {
		metadata->size = 0;
		curlFreeHandler(reply->offset);
		return res;
	}
	if (!IS_HTTP_SUCCESS(http_code)) {
		res = -http_code;
		goto fail;
	}
	if (reply->len < sizeof(CecSlotHeader)) {
		metadata->size = 0;
		curlFreeHandler(reply->offset);
		return res;
	}
	CecMessageHeader* msg = (CecMessageHeader*)(reply->ptr + sizeof(CecSlotHeader));
	CecSlotHeader* slot = (CecSlotHeader*)reply->ptr;
	metadata->send_method = msg->send_method;
	metadata->size = slot->size;
	slotinfo->slots[i] = malloc(slot->size);
	if (!slotinfo->slots[i]) {
		res = ERROR_MISSING_SLOT_META;
		goto fail;
	}

	memcpy(slotinfo->slots[i], reply->ptr, slot->size);

	curlFreeHandler(reply->offset);
	return res;
fail:
	metadata->size = 0;
	curlFreeHandler(reply->offset);
	return res;
}

Result doSlotExchange(void) {
	Result res = 0;
	TitleExtraInfo title_extra_info[12];
	memset(&title_extra_info, 0, sizeof(TitleExtraInfo)*12);
	SlotInfo slotinfo;
	memset(&slotinfo, 0, sizeof(SlotInfo));
	char* error_origin = "none";
	// first we fetch the mboxlist, extend it and upload it
	{
		CecMboxListHeaderWithCapacities* mbox_list = malloc(sizeof(CecMboxListHeaderWithCapacities));
		if (!mbox_list) {
			res = _e(ERROR_OUT_OF_MEMORY);
			goto fail;
		}
		memset(mbox_list, 0, sizeof(CecMboxListHeaderWithCapacities));
		res = _e_cec(cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mbox_list->header), (u8*)&mbox_list->header));
		error_origin = "reading mbox list";
		if (R_FAILED(res)) {
			free(mbox_list);
			goto fail;
		}
		clearIgnoredTitles(&mbox_list->header);
		// now fill in the capacities
		for (size_t i = 0; i < mbox_list->header.num_boxes; i++) {
			u32 title_id = strtol((const char*)mbox_list->header.box_names[i], NULL, 16);
			CecBoxInfoHeader boxinfo;
			res = _e_cec(cecdOpenAndRead(title_id, CEC_PATH_INBOX_INFO, sizeof(boxinfo), (u8*)&boxinfo));
			if (R_FAILED(res)) {
				free(mbox_list);
				goto fail;
			}
			mbox_list->capacities[i] = boxinfo.max_num_messages - boxinfo.num_messages;
			
			// now we fetch the title name
			res = _e_cec(cecdOpenAndRead(title_id, CECMESSAGE_BOX_TITLE, 198, (u8*)mbox_list->title_names[i]));
			if (R_FAILED(res)) {
				free(mbox_list);
				goto fail;
			}
			
			// and now the hmac key
			CecMBoxInfoHeader info_header;
			res = _e_cec(cecdOpenAndRead(title_id, CEC_PATH_MBOX_INFO, sizeof(info_header), (u8*)&info_header));
			if (R_FAILED(res)) {
				free(mbox_list);
				goto fail;
			}
			memcpy(mbox_list->hmac_keys[i], info_header.hmac_key, 32);
			
			title_extra_info[i].title_id = title_id;
		}
		char url[50];
		snprintf(url, 50, "%s/outbox/mboxlist_ext2", BASE_URL);
		res = _e(httpRequest("POST", url, sizeof(CecMboxListHeaderWithCapacities), (u8*)mbox_list, 0, 0));
		free(mbox_list);
		error_origin = "sending mboxlist ext";
		if (R_FAILED(res)) goto fail;
	}

	// get cecd into the spr state
	error_origin = "Getting cecd into spr state";
	res = _e_cec(waitForCecdState(false, CEC_COMMAND_OVER_BOSS, CEC_STATE_ABBREV_INACTIVE));
	if (R_FAILED(res)) goto fail;

	// now we init spr stuffs
	res = _e_cec(cecdSprCreate());
	error_origin = "cecd spr create";
	if (R_FAILED(res)) goto fail;
	res = _e_cec(cecdSprInitialise());
	error_origin = "cecd spr init";
	if (R_FAILED(res)) goto fail;

	// Fetch the metadata

	u32 slots_total;
	error_origin = "cecd spr get slots metadata";
	res = _e_cec(cecdSprGetSlotsMetadata(sizeof(SlotMetadata)*12, slotinfo.metadata, &slots_total));
	if (R_FAILED(res)) goto fail;
	log_line_start(INFO, "Uploading outboxes (%ld/%d)... ", slots_total, numUsedTitles());

	// Upload all slots
	for (int i = 0; i < slots_total; i++) {
		TitleExtraInfo* extra = 0;
		for (int j = 0; j < 12; j++) {
			if (slotinfo.metadata[i].title_id == title_extra_info[j].title_id) {
				extra = &title_extra_info[j];
				break;
			}
		}
		if (!extra) {
			continue; // the slot was disabled
		}
		Result res2 = uploadSlot(&slotinfo.metadata[i]);
		if (R_FAILED(res2)) {
			log_line_continue("-");
		} else {
			log_line_continue("=");
		}
		error_origin = "upload slot";
		res = _e_cec(cecdSprSetTitleSent(slotinfo.metadata[i].title_id, !R_FAILED(res2)));
		if (res2 == -400) { // we still want to continue if it was http 400
			res2 = 0;
		}
		if (R_FAILED(res) || R_FAILED(res = res2)) goto fail;
	}
	// we are done sending things
	res = _e_cec(cecdSprFinaliseSend());
	error_origin = "finalise send";
	if (R_FAILED(res)) goto fail;
	log_line_finish("Done");
	log_line_start(INFO, "Downloading inboxes (%ld/%d)... ", slots_total, numUsedTitles());

	// time to start download!
	res = _e_cec(cecdSprStartRecv());
	error_origin = "start recv";
	if (R_FAILED(res)) goto fail;

	// download all slots
	for (int i = 0; i < slots_total; i++) {
		// make sure the slot isn't disabled
		bool found = false;
		for (int j = 0; j < 12; j++) {
			if (slotinfo.metadata[i].title_id == title_extra_info[j].title_id) {
				found = true;
				break;
			}
		}
		if (!found) {
			continue; // the slot was disabled
		}
		res = downloadSlot(i, &slotinfo);
		error_origin = "download slot";
		if (R_FAILED(res)) goto fail;
	}

	// notify cecd of the slots
	res = _e_cec(cecdSprAddSlotsMetadata(sizeof(SlotMetadata)*slots_total, (u8*)slotinfo.metadata));
	error_origin = "add slots metadata";
	if (R_FAILED(res)) goto fail;

	// add all slots
	error_origin = "add slots";
	int slot_new_data_num = 0;
	for (int i = 0; i < slots_total; i++) {
		// make sure the slot isn't disabled
		bool found = false;
		for (int j = 0; j < 12; j++) {
			if (slotinfo.metadata[i].title_id == title_extra_info[j].title_id) {
				found = true;
				break;
			}
		}
		if (!found) {
			continue; // the slot was disabled
		}
		if (slotinfo.metadata[i].size == 0 || slotinfo.slots[i] == 0) {
			log_line_continue("=");
			continue;
		}
		slot_new_data_num++;
		res = _e_cec(cecdSprAddSlot(slotinfo.metadata[i].title_id, ((CecSlotHeader*)(slotinfo.slots[i]))->size, slotinfo.slots[i]));
		saveSlotInLog(slotinfo.slots[i]);
		if (R_FAILED(res)) {
			log_line_continue("-");
			goto fail;
		} else {
			log_line_continue("=");
		}
	}

	res = _e_cec(cecdSprFinaliseRecv());
	error_origin = "cecd spr finalise recv";
	if (R_FAILED(res)) goto fail;
	res = _e_cec(cecdSprDone(true));
	error_origin = "cecd spr done";
	if (R_FAILED(res)) goto fail;

	log_line_finish("Done (%d)", slot_new_data_num);

	goto cleanup;
fail:
	cecdSprDone(false);
	logln(ERROR, "(%s): %08lx", error_origin, res);
cleanup:
	for (int i = 0; i < 12; i++) {
		if (slotinfo.slots[i]) {
			free(slotinfo.slots[i]);
			slotinfo.slots[i] = 0;
		}
	}
	Result res_bak = res;
	// get cecd into the normal state
	res = waitForCecdState(true, CEC_COMMAND_STOP, CEC_STATE_ABBREV_IDLE);
	if (R_FAILED(res_bak)) {
		return res_bak;
	}
	return res;
}

Result getLocation(void) {
	Result res;
	CurlReply* reply;
	char url[80];
	snprintf(url, 80, "%s/location/current/info", BASE_URL);
	res = httpRequest("GET", url, 0, 0, &reply, 0);
	if (R_FAILED(res)) goto cleanup;
	int http_code = res;
	if (http_code == 200) {
		QrBuffer buffer;
		qr_buffer_new(&buffer, reply->ptr, reply->len);
		// check version
		if (qr_read_u32(&buffer) != 1) {
			res = ERROR_INVALID_SERVER_RESPONSE;
			goto cleanup;
		}
		memset(&location, 0, sizeof(location));
		location.id = qr_read_s32(&buffer);
		qr_read_object(&buffer, location.uuid, 16);
		location.have_image = qr_read_u8(&buffer);
		qr_read_align(&buffer, 4);
		qr_read_object(&buffer, location.image_hash, 0x20);
		qr_read_object(&buffer, &location.time_start, sizeof(location.time_start));
		qr_read_object(&buffer, &location.time_end, sizeof(location.time_end));
		location.time_remaining = qr_read_u32(&buffer);
		qr_read_string(&buffer, location.name, 100);
		qr_read_align(&buffer, 4);
		qr_read_string(&buffer, location.artist_name, 100);
		
		cache_current_location_image();
	} else {
		res = -http_code;
	}
cleanup:
	curlFreeHandler(reply->offset);
	return res;
}

Result setLocation(int location) {
	Result res;
	if (config.last_location == location) return ERROR_SAME_LOCAION_TWICE;
	
	// first check if we have any streetpass games enabled
	CecMboxListHeaderWithCapacities mbox_list;
	res = _e(cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mbox_list.header), (u8*)&mbox_list.header));
	if (R_FAILED(res)) return res;
	clearIgnoredTitles(&mbox_list.header);
	if (mbox_list.header.num_boxes == 0) return ERROR_NO_STREETPASS_GAMES;

	// now actually ask the server to enter the location
	char url[80];
	snprintf(url, 80, "%s/location/%d/enter", BASE_URL, location);
	res = httpRequest("PUT", url, 0, 0, 0, 0);
	if (R_FAILED(res)) {
		logln(ERROR, "Failed to enter location %d: %ld", location, res);
		return res;
	}
	config.last_location = location;
	configWrite();
	logln(INFO, "Entered location %d!", location);
	cache_current_location_image();
	return res;
}

Result setEventLocation(u8 uuid[16]) {
	Result res;
	// first check if we have any streetpass games enabled
	CecMboxListHeaderWithCapacities mbox_list;
	res = _e(cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mbox_list.header), (u8*)&mbox_list.header));
	if (R_FAILED(res)) return res;
	clearIgnoredTitles(&mbox_list.header);
	if (mbox_list.header.num_boxes == 0) return ERROR_NO_STREETPASS_GAMES;

	// now actually ask the server to enter the location
	char uuidstr[37];
	format_uuid(uuidstr, uuid);
	char url[80];
	snprintf(url, 80, "%s/location/%s/enter", BASE_URL, uuidstr);
	res = httpRequest("PUT", url, 0, 0, 0, 0);
	if (R_FAILED(res)) {
		logln(ERROR, "Failed to event enter location %s: %ld", uuidstr, res);
	}
	cache_current_location_image();
	return res;
}

static s32 main_thread_prio_s = 0;
s32 main_thread_prio(void) {
	return main_thread_prio_s;
}
void init_main_thread_prio(void) {
	svcGetThreadPriority(&main_thread_prio_s, CUR_THREAD_HANDLE);
}

static volatile int dl_inbox_status = 1;
static bool dl_loop_running = true;
Thread bg_loop_thread = 0;
void triggerDownloadInboxes(void) {
	while (dl_inbox_status != 0) svcSleepThread((u64)1000000 * 100);
	dl_inbox_status = 1;
}

Result doSlotExchangeRetry(void) {
	int count = 0;
	while(true) {
		Result res = doSlotExchange();
		if (R_FAILED(res)) {
			if (R_IS_CEC_RESTART(res)) {
				count++;
				if (count < 20) {
					logln(INFO, "Retrying slot exchange...");
					svcSleepThread(10e3);
					continue;
				}
			}
			return res;
		}
		return 0;
	}
}

void bgLoop(void* p) {
	do {
		dl_inbox_status = 2;
		_e(doSlotExchangeRetry());
		dl_inbox_status = 0;
		for(int i = 0; i < 10*60*5; i++) {
			svcSleepThread((u64)1000000 * 100);
			if (dl_inbox_status == 1 || !dl_loop_running) break;
		}
	} while(dl_loop_running);
}

void bgLoopInit(void) {
	bg_loop_thread = threadCreate(bgLoop, NULL, 8*1024, main_thread_prio()+1, -2, false);
}

void bgLoopExit(void) {
	dl_loop_running = false;
	if (bg_loop_thread) {
		threadFree(bg_loop_thread);
	}
}
