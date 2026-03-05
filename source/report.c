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

#include "report.h"
#include "api.h"
#include "boss.h"
#include "cecd.h"
#include "config.h"
#include "utils.h"
#include "strings.h"
#include "curl-handler.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include "integration.h"

#include <errno.h>

#define LOG_DIR "sdmc:/config/netpass/log"
#define LOG_SPR_DIR "sdmc:/config/netpass/log_spr"
#define LOG_LIST_TMP "sdmc:/config/netpass/log_list.tmp"
#define LOG_ENTRY_TMP "sdmc:/config/netpass/log_entry.tmp"
#define LOG_ENTRY_DEC_TMP "sdmc:/config/netpass/log_entry_dec.tmp"


#define MAX_REPORT_ENTRIES_LEN 128
#define REPORT_LIST_MAGIC 0x454C524e

#define SETUP_ENTRY(a, x) a* body = (a*)(buf + msg->total_header_size); \
	entry->data = malloc(sizeof(x)); \
	if (!entry->data) break; \
	x* data = (x*)entry->data; \
	memset(data, 0, sizeof(x));

typedef struct {
		u32 offset;
		u32 size;
		u32 checksum;
		char name[8];
} BPK1BlockHeader;

typedef struct {
		u32 magic; // "BPK1"
		u32 num_blocks;
} BPK1Header;

Result loadReportList(FILE** file) {
	char url[100];
	Result res;
	snprintf(url, 100, "%s/report/list", BASE_URL);
	res = httpRequest("GET", url, 0, NULL, NULL, LOG_LIST_TMP);
	if (R_FAILED(res)) return res;
	FILE* f;
	f = fopen(LOG_LIST_TMP, "r");
	if (!f) return _e_errno();
	*file = f;
	return 0;
}

Result loadReportMessagesBoss(ReportMessages** msgs_out, u64 data_id, u32 title_id) {
	char url[100];
	Result res;
	ReportMessages* msgs = 0;
	u8* buf = 0;
	FILE* f = 0;
	FILE* fd = 0;
	
	snprintf(url, 100, "%s/report/get_boss/%08lx/%llu", BASE_URL, title_id, data_id);
	res = httpRequest("GET", url, 0, NULL, NULL, LOG_ENTRY_TMP);
	if (R_FAILED(res)) return res;
	logln(INFO, "title id: %08lx", title_id);
	
	f = fopen(LOG_ENTRY_TMP, "r");
	if (!f) goto fail_errno;
	
	size_t msgs_size = sizeof(ReportMessages) + sizeof(ReportMessagesEntry);
	msgs = malloc(msgs_size);
	if (!msgs) goto fail_errno;
	memset(msgs, 0, msgs_size);
	msgs->count = 1;
	msgs->source_id = 0x504E; // "NP"
	msgs->source_name = "NetPass";
	
	ReportMessagesEntry* entry = &msgs->entries[0];
	entry->title_id = title_id;
	
	switch (title_id) {
		case TITLE_SWAPDOODLE: {
			fd = fopen(LOG_ENTRY_DEC_TMP, "w+");
			if (!fd) goto fail_errno;
			if (!blz_decompress_file(f, fd)) goto fail_errno;

			fseek(fd, 0, SEEK_SET);
			char magic[5];
			fread(magic, 4, 1, fd);
			magic[4] = 0;
			u32 num_blocks = 0;
			if (fread(&num_blocks, sizeof(num_blocks), 1, fd) != 1) goto fail_errno;
			fseek(fd, 0x38, SEEK_CUR);
			size_t curseek = ftell(fd);
			size_t startseek = curseek;
			u32 num_thumbs = 0;
			for (int i = 0; i < num_blocks; i++) {
				BPK1BlockHeader block_header;
				fseek(fd, curseek, SEEK_SET);
				if (fread(&block_header, sizeof(block_header), 1, fd) != 1) goto fail_errno;
				curseek = ftell(fd);
				if (strcmp(block_header.name, "MIISTD1") == 0) {
					fseek(fd, block_header.offset, SEEK_SET);
					entry->mii = malloc(sizeof(MiiData));
					if (entry->mii) {
						int reads = 0;
						if ((reads = fread(entry->mii, 1, sizeof(MiiData), fd)) != sizeof(MiiData)) {
							free(entry->mii);
							entry->mii = NULL;
						}
					} else {
						res = ERROR_OUT_OF_MEMORY;
						goto fail;
					}
				} else if (strcmp(block_header.name, "THUMB2") == 0) {
					num_thumbs++;
				}
			}
			curseek = startseek;
			
			size_t data_size = sizeof(ReportMessageEntrySwapdoodle) + (sizeof(ReportMessageEntrySwapdoodleThumb) * num_thumbs);
			entry->data = malloc(data_size);
			if (!entry->data) goto fail_errno;
			ReportMessageEntrySwapdoodle* data = (ReportMessageEntrySwapdoodle*)entry->data;
			memset(data, 0, data_size);
			
			data->count = num_thumbs;
			
			int thumb_counter = 0;
			for (int i = 0; i < num_blocks; i++) {
				BPK1BlockHeader block_header;
				fseek(fd, curseek, SEEK_SET);
				if (fread(&block_header, sizeof(block_header), 1, fd) != 1) goto fail_errno;
				curseek = ftell(fd);
				if (strcmp(block_header.name, "THUMB2") == 0) {
					fseek(fd, block_header.offset, SEEK_SET);
					u8* buf = malloc(block_header.size);
					if (!buf) goto thumb_fail;
					if (fread(buf, block_header.size, 1, fd) != 1) goto thumb_fail;
					data->thumbs[thumb_counter].size = block_header.size;
					data->thumbs[thumb_counter].data = buf;
					thumb_counter++;
				}
			}
		}
	}
	
	if (buf) free(buf);
	if (f) fclose(f);
	if (fd) fclose(fd);
	*msgs_out = msgs;
	return 0;
thumb_fail:
	res = ERROR_OUT_OF_MEMORY;
	ReportMessageEntrySwapdoodle* data = (ReportMessageEntrySwapdoodle*)entry->data;
	for (int i = 0; i < data->count; i++) {
		if (data->thumbs[i].data) free(data->thumbs[i].data);
	}
	goto fail;
fail_errno:
	res = _e_errno();
fail:
	if (buf) free(buf);
	if (msgs) free(msgs);
	if (f) fclose(f);
	if (fd) fclose(fd);
	if (!R_FAILED(res)) res = ERROR_UNKNOWN;
	return res;
}

Result loadReportMessagesCec(ReportMessages** msgs_out, u64 mac, u32 transfer_id) {
	char url[100];
	Result res;
	u8* buf = 0;
	ReportMessages* msgs = 0;
	
	snprintf(url, 100, "%s/report/get_cec/%012llx/%08lx", BASE_URL, mac, transfer_id);
	res = httpRequest("GET", url, 0, NULL, NULL, LOG_ENTRY_TMP);
	if (R_FAILED(res)) return res;
	FILE* f;
	f = fopen(LOG_ENTRY_TMP, "r");
	if (!f) return _e_errno();
	u32 num_messages;
	if (fread(&num_messages, sizeof(num_messages), 1, f) != 1) goto fail_errno;
	
	buf = malloc(MAX_MESSAGE_SIZE);
	if (!buf) goto fail_errno;
	size_t msgs_size = sizeof(ReportMessages) + (sizeof(ReportMessagesEntry) * num_messages);
	msgs = malloc(msgs_size);
	if (!msgs) goto fail_errno;
	memset(msgs, 0, msgs_size);
	
	msgs->count = num_messages;
	msgs->source_id = 0;
	msgs->source_name = 0;
	
	u16 source_ident = 0;
	for (int i = 0; i < num_messages; i++) {
		if (fread(buf, sizeof(CecMessageHeader), 1, f) != 1) goto fail_errno;
		
		CecMessageHeader* msg = (CecMessageHeader*)buf;
		if (msg->magic != 0x6060) {
			res = ERROR_INVALID_MESSAGE;
			goto fail;
		}
		if (fread(buf + sizeof(CecMessageHeader), msg->message_size - sizeof(CecMessageHeader), 1, f) != 1) goto fail_errno;
		// we have the full message loaded into buf now
		if (!source_ident) {
			source_ident = msg->padding_sourceident;
		}
		// we have the file now in buf, time to populate the specific entry
		ReportMessagesEntry* entry = &msgs->entries[i];
		entry->title_id = msg->title_id;
		// fetch the mii name, if any
		CFPB* cfpb = (CFPB*)memsearch(buf + msg->total_header_size, msg->message_size, (u8*)"CFPB", 4);
		if (cfpb) {
			entry->mii = malloc(sizeof(MiiData));
			if (entry->mii) {
				Result r = decryptMii(&cfpb->nonce, entry->mii);
				if (R_FAILED(r) || entry->mii->version != 3) {
					// since we are just scanning payloads for a mii
					// we do not trigger an error if a mii fails to decrypt here
					free(entry->mii);
					entry->mii = 0;
				}
			} else {
				res = ERROR_OUT_OF_MEMORY;
				goto fail;
			}
		}

		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				u8 needle[2] = {0xFF, 0xD8};
				u8* ptr = memsearch(buf + msg->total_header_size, msg->message_size, needle, 2);
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
		if (R_FAILED(res)) goto fail;
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
	}
	fclose(f);
	free(buf);
	
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
	
	*msgs_out = msgs;

	return 0;
fail_errno:
	res = _e_errno();
fail:
	if (buf) free(buf);
	if (msgs) free(msgs);
	fclose(f);
	if (!R_FAILED(res)) res = ERROR_UNKNOWN;
	return res;
}

Result loadReportMessages(ReportMessages **msgs, u64 id, u32 misc_id, ReportType report_type) {
	switch (report_type) {
		case REPORT_TYPE_CEC:
			return loadReportMessagesCec(msgs, id, misc_id);
		case REPORT_TYPE_BOSS:
			return loadReportMessagesBoss(msgs, id, misc_id);
	}
	return ERROR_UNKNOWN;
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
			if (entry->title_id == TITLE_SWAPDOODLE) {
				ReportMessageEntrySwapdoodle* data = (ReportMessageEntrySwapdoodle*)entry->data;
				for (int i = 0; i < data->count; i++) {
					if (data->thumbs[i].data) free(data->thumbs[i].data);
				}
			}
			free(entry->data);
			entry->data = 0;
		}
	}
}

void reportInit(void) {
	rmdir_r(LOG_DIR);
	rmdir_r(LOG_SPR_DIR);
	mkdir_p(LOG_LIST_TMP);
}

void reportExit(void) {
	//unlink(LOG_LIST_TMP);
	//unlink(LOG_ENTRY_TMP);
	//unlink(LOG_ENTRY_DEC_TMP);
}
