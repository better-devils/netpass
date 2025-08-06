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

#include "report.h"
#include "cecd.h"
#include "config.h"
#include "utils.h"
#include "strings.h"
#include "curl-handler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include "integration.h"

#define LOG_DIR "sdmc:/config/netpass/log/"
#define LOG_INDEX "sdmc:/config/netpass/log/index.nrle"
#define LOG_SPR_DIR "sdmc:/config/netpass/log_spr/"

#define MAX_REPORT_ENTRIES_LEN 128

#define SETUP_ENTRY(a, x) a* body = (a*)(((u8*)buf) + buf->total_header_size); \
	entry->data = malloc(sizeof(x)); \
	if (!entry->data) break; \
	x* data = (x*)entry->data; \
	memset(data, 0, sizeof(x));

ReportList* loadReportList(void) {
	FILE* f = fopen(LOG_INDEX, "r");
	if (!f) {
		_e_errno();
		return NULL;
	}

	ReportListHeader header;
	fread_blk(&header, sizeof(ReportListHeader), 1, f);
	if (header.magic != 0x454C524e || header.version != 1) {
		_e(ERROR_BAD_REPORT_LIST);
		return NULL;
	}
	fseek(f, 0, SEEK_SET);
	size_t list_file_size = sizeof(ReportListHeader) + header.max_size * sizeof(ReportSendPayload);
	
	ReportList* list = memalign(4, list_file_size);
	if (!list) return NULL;
	fread_blk(list, list_file_size, 1, f);
	fclose(f);
	return list;
}

bool loadReportMessages(ReportMessages* msgs, u32 transfer_id) {
	memset(msgs, 0, sizeof(ReportMessages));

	char dirname[100];
	snprintf(dirname, 100, "%s%lx", LOG_DIR, transfer_id);
	size_t path_len = strlen(dirname);

	DIR* d = opendir(dirname);
	if (!d) {
		_e_errno();
		return false;
	}
	struct dirent *p;
	CecMessageHeader* buf = malloc(MAX_MESSAGE_SIZE);
	if (!buf) {
		_e(ERROR_OUT_OF_MEMORY);
		return false;
	}
	u16 source_ident = 0;
	while ((p=readdir(d)) && msgs->count < 12) {
		int fname_len = path_len + strlen(p->d_name) + 2;
		char* fname = malloc(fname_len);
		if (!fname) {
			_e(ERROR_OUT_OF_MEMORY);
			free(buf);
			return false;
		}
		snprintf(fname, fname_len, "%s/%s", dirname, p->d_name);
		// we found a file
		FILE* f = fopen(fname, "rb");
		if (!f) goto cont_loop;
		fread_blk(buf, MAX_MESSAGE_SIZE, 1, f);
		if (buf->magic != 0x6060) {
			_e(ERROR_INVALID_MESSAGE);
			fclose(f);
			goto cont_loop;
		}
		if (!source_ident) {
			source_ident = buf->padding_sourceident;
		}
		fclose(f);
		// we have the file now in buf, time to populate the specific entry
		ReportMessagesEntry* entry = &msgs->entries[msgs->count];
		entry->title_id = buf->title_id;
		// fetch the mii name, if any
		CFPB* cfpb = (CFPB*)memsearch(((u8*)buf) + buf->total_header_size, buf->message_size, (u8*)"CFPB", 4);
		if (cfpb) {
			entry->mii = malloc(sizeof(MiiData));
			if (entry->mii) {
				Result r = decryptMii(&cfpb->nonce, entry->mii);
				if (R_FAILED(r) || entry->mii->version != 3) {
					if (R_FAILED(r)) {
						_e(r);
					} else {
						_e(ERROR_INVALID_MII);
					}
					free(entry->mii);
					entry->mii = 0;
				}
			} else {
				_e(ERROR_OUT_OF_MEMORY);
			}
		}

		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				u8 needle[2] = {0xFF, 0xD8};
				u8* ptr = memsearch(((u8*)buf) + buf->total_header_size, buf->message_size, needle, 2);
				if (!ptr) break;
				ptr -= 0x68 + 4;
				u32 size = ((ReportMessagesEntryLetterBox*)ptr)->jpeg_size;

				entry->data = malloc(size);
				if (!entry->data) break;
				memcpy(entry->data, &((ReportMessagesEntryLetterBox*)ptr)->jpegs, size);
				break;
			}
			case TITLE_MARIO_KART_7: {
				SETUP_ENTRY(CecMessageBodyMarioKart7, ReportMessageEntryMarioKart7);
				utf16_to_utf8((u8*)data->greeting, body->message, sizeof(data->greeting)-1);
				break;
			}
			case TITLE_MII_PLAZA: {
				SETUP_ENTRY(CecMessageBodyMiiPlaza, ReportMessageEntryMiiPlaza);
				u8 lang = get_nintendo_language();
				utf16_to_utf8((u8*)data->last_game, body->title[lang].short_description, sizeof(data->last_game)-1);
				utf16_to_utf8((u8*)data->country, body->country[lang].name, sizeof(data->country)-1);
				utf16_to_utf8((u8*)data->region, body->region[lang].name, sizeof(data->region)-1);
				utf16_to_utf8((u8*)data->greeting, body->message, sizeof(data->greeting)-1);
				u8* mac = getMacBuf();
				for (int i = 0; i < 0x10; i++) {
					if (!memcmp(mac, body->reply_list[i].mac, 6)) {
						utf16_to_utf8((u8*)data->custom_message, body->reply_msg[i].message, sizeof(data->custom_message)-1);
						utf16_to_utf8((u8*)data->custom_reply, body->replied_msg[i].message, sizeof(data->custom_reply)-1);
						break;
					}
				}
				break;
			}
			case TITLE_TOMODACHI_LIFE: {
				SETUP_ENTRY(CecMessageBodyTomodachiLife, ReportMessageEntryTomodachiLife);
				utf16_to_utf8((u8*)data->island_name, body->island_name, sizeof(data->island_name)-1);
			};
		}

		// we don't need buf anymore so we can use it now to fetch the game name
		memset(buf, 0, 300);
		Result res = cecdOpenAndRead(entry->title_id, CECMESSAGE_BOX_TITLE, 198, (u8*)buf);
		if (R_FAILED(res)) goto cont_loop;
		char* game_name = ((char*)buf) + 200;
		memset(game_name, 0, 100);
		// SAFETY: utf16_to_utf8 does not write a zero terminator, so we memset above
		int len = utf16_to_utf8((u8*)game_name, (u16*)buf, 100 - 1);
		if (len > -1) {
			entry->name = malloc(len+1);
			memcpy(entry->name, game_name, len+1);
		} else {
			// Allocate an empty string for cleanup code
			entry->name = malloc(1);
			*entry->name = 0;
		}
		msgs->count++;
	cont_loop:
		free(fname);
	}
	free(buf);
	closedir(d);
	
	msgs->source_name = 0;
	msgs->source_id = source_ident;
	IntegrationList* list = get_integration_list();
	if (list) {
		if (source_ident == 0x504E) { // "NP"
			msgs->source_name = "NetPass";
		} else {
			for (int i = 0; i < list->header.count; i++) {
				if (list->entries[i].source_ident == source_ident) {
					msgs->source_name = list->entries[i].name;
					break;
				}
			}
		}
	}
	

	return true;
}

void freeReportMessages(ReportMessages* msgs) {
	if (!msgs) return;
	for (int i = 0; i < msgs->count; i++) {
		ReportMessagesEntry* entry = &msgs->entries[i];
		if (entry->name) {
			free(entry->name);
			entry->name = 0;
		}
		if (entry->mii) {
			free(entry->mii);
			entry->mii = 0;
		}
		if (entry->data) {
			free(entry->data);
			entry->data = 0;
		}
	}
}

void saveSlotInLog(CecSlotHeader* slot) {
	u8* ptr = ((u8*)slot) + sizeof(CecSlotHeader);
	for (int i = 0; i < slot->message_count; i++) {
		CecMessageHeader* msg = (CecMessageHeader*)ptr;
		saveMsgInLog(msg);
		ptr += msg->message_size;
	}
}

void saveMsgInLog(CecMessageHeader* msg) {
	ReportList* list;
	FILE* f = fopen(LOG_INDEX, "rb");
	if (!f) {
		// ok, file is empty, we have to create it
		f = fopen(LOG_INDEX, "wb");
		if (!f) {
			_e_errno();
			return;
		}
		list = memalign(4, sizeof(ReportListHeader) + sizeof(ReportListEntry) * MAX_REPORT_ENTRIES_LEN);
		if (!list) {
			_e(ERROR_OUT_OF_MEMORY);
			fclose(f);
			return;
		}
		memset(list, 0, sizeof(ReportListHeader) + sizeof(ReportListEntry) * MAX_REPORT_ENTRIES_LEN);
		list->header.magic = 0x454C524e;
		list->header.version = 1;
		list->header.max_size = MAX_REPORT_ENTRIES_LEN;
		list->header.cur_size = 0;
		fwrite_blk(list, sizeof(ReportList), 1, f);
		free(list);
		fclose(f);
		f = fopen(LOG_INDEX, "rb");
		if (!f) {
			_e_errno();
			return;
		}
	}
	size_t list_file_size;
	{
		ReportListHeader header;
		fread_blk(&header, sizeof(ReportListHeader), 1, f);
		if (header.magic != 0x454C524E || header.version != 1) {
			_e(ERROR_BAD_REPORT_LIST);
			return;
		}
		fseek(f, 0, SEEK_SET);
		list_file_size = sizeof(ReportListHeader) + header.max_size * sizeof(ReportSendPayload);
		list = memalign(4, list_file_size);
		if (!list) {
			_e(ERROR_OUT_OF_MEMORY);
			return;
		}
		fread_blk(list, list_file_size, 1, f);
		fclose(f);
	}
	int found_i = -1;
	// find if the transfer id already exists
	for (int i = 0; i < list->header.cur_size; i++) {
		if (list->entries[i].transfer_id == msg->transfer_id) {
			found_i = i;
			break;
		}
	}
	char* b64name = b64encode(msg->message_id, 8);
	char filename[100];
	snprintf(filename, 100, "%s%lx/_%s", LOG_DIR, msg->transfer_id, b64name);
	free(b64name);
	bool edited = false;
	if (found_i < 0) {
		// we have to add a new entry!
		if (list->header.max_size == list->header.cur_size) {
			// uho, all is full, gotta the first half of the list
			int i = 0;
			for (; i < MAX_REPORT_ENTRIES_LEN / 2; i++) {
				u32 rm_batch = list->entries[i].transfer_id;
				char rm_dirname[100];
				snprintf(rm_dirname, 100, "%s%lx", LOG_DIR, rm_batch);
				rmdir_r(rm_dirname);
				list->header.cur_size--;
			}
			memmove(list->entries, ((u8*)list->entries) + sizeof(ReportListEntry)*i, list->header.cur_size * sizeof(ReportListEntry));
		}
		ReportListEntry* e = &list->entries[list->header.cur_size];
		e->transfer_id = msg->transfer_id;
		memcpy(&e->received, &msg->received, sizeof(CecTimestamp));
		found_i = list->header.cur_size;
		list->header.cur_size++;
		edited = true;
	}
	ReportListEntry* e = &list->entries[found_i];
	if (msg->title_id == TITLE_MII_PLAZA) {
		CecMessageBodyMiiPlaza* body = (CecMessageBodyMiiPlaza*)(((u8*)msg) + msg->total_header_size);
		static const int cfpb_offset = 0x36bc;
		static const int cfpb_size = 0x88;
		if (msg->message_size > msg->total_header_size + cfpb_offset + cfpb_size) {
			if (body->cfpb.magic == 0x42504643) {
				int prev_mii_id = e->mii.version == 3 ? e->mii.mii_id : 0;
				Result r = decryptMii(&body->cfpb.nonce, &e->mii);
				if (R_FAILED(r)) {
					_e(r);
				} else if (prev_mii_id != e->mii.mii_id) {
					edited = true;
				}
			}
		}
	} else if (e->mii.version != 3) {
		// search if there is a mii in this payload
		CFPB* cfpb = (CFPB*)memsearch(((u8*)msg) + msg->total_header_size, msg->message_size, (u8*)"CFPB", 4);
		if (cfpb) {
			Result r = decryptMii(&cfpb->nonce, &e->mii);
			if (R_FAILED(r)) {
				_e(r);
			} else {
				edited = true;
			}
		}
	}

	if (edited) {
		f = fopen(LOG_INDEX, "wb");
		if (!f) {
			_e_errno();
			goto error;
		}
		fwrite_blk(list, list_file_size, 1, f);
		fclose(f);
	}
	mkdir_p(filename);
	f = fopen(filename, "wb");
	if (!f) {
		_e_errno();
		goto error;
	}
	fwrite_blk(msg, msg->message_size, 1, f);
	fclose(f);

error:
	free(list);
}

Result reportGetSomeMsgHeader(CecMessageHeader* msg, u32 transfer_id) {
	msg->magic = 0;

	char dirname[100];
	snprintf(dirname, 100, "%s%lx", LOG_DIR, transfer_id);
	size_t path_len = strlen(dirname);

	DIR* d = opendir(dirname);
	if (!d) {
		_e_errno();
		return ERROR_ERRNO;
	}

	struct dirent *p;
	while ((p=readdir(d))) {
		int fname_len = path_len + strlen(p->d_name) + 2;
		char* fname = malloc(fname_len);
		if (!fname) {
			_e_errno();
			closedir(d);
			return ERROR_ERRNO;
		}
		snprintf(fname, fname_len, "%s/%s", dirname, p->d_name);
		// we found a file
		FILE* f = fopen(fname, "rb");
		if (f) {
			fread_blk(msg, sizeof(CecMessageHeader), 1, f);
			fclose(f);
			if (msg->magic == 0x6060 && msg->transfer_id == transfer_id) {
				free(fname);
				break;
			} else {
				msg->magic = 0;
			}
		}
		free(fname);
	}
	closedir(d);

	if (!msg->magic) return -2; // nothing in directory

	return 0;
}

void reportInit(void) {
	mkdir_p(LOG_DIR);
	mkdir_p(LOG_SPR_DIR);

	DIR* d = opendir(LOG_SPR_DIR);
	if (!d) return;
	struct dirent* p;
	char filename[200];
	bool has_spr_passes = false;
	while ((p = readdir(d))) {
		size_t len = strlen(LOG_SPR_DIR) + strlen(p->d_name) + 1;
		struct stat statbuf;
		snprintf(filename, len, "%s%s", LOG_SPR_DIR, p->d_name);
		if (stat(filename, &statbuf) && !S_ISDIR(statbuf.st_mode)) continue;
		if (!has_spr_passes) {
			has_spr_passes = true;
			printf("Add SPR passes ");
		}
		FILE* f = fopen(filename, "rb");
		if (!f) continue;
		fseek(f, 0, SEEK_END);
		size_t filesize = ftell(f);
		rewind(f);
		if (filesize < sizeof(CecSlotHeader)) {
			fclose(f);
			printf("/");
			unlink(filename);
			continue;
		}
		CecSlotHeader slot;
		fread_blk(&slot, sizeof(CecSlotHeader), 1, f);
		if (slot.size > MAX_SLOT_SIZE) {
			fclose(f);
			printf("S");
			unlink(filename);
			continue;
		}
		CecSlotHeader* buf_slot = malloc(slot.size);
		if (!buf_slot) {
			printf("B");
			fclose(f);
			unlink(filename);
			continue;
		}
		rewind(f);
		fread_blk(buf_slot, slot.size, 1, f);
		fclose(f);
		printf("=");
		saveSlotInLog(buf_slot);
		free(buf_slot);
		unlink(filename);
	}
	closedir(d);
	if (has_spr_passes) printf(" Done\n");
}
