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
#include <3ds/types.h>

typedef enum {
	TITLE_LETTER_BOX     = 0x051600,
	TITLE_MII_PLAZA      = 0x020800,
	TITLE_MARIO_KART_7   = 0x030600,
	TITLE_TOMODACHI_LIFE = 0x08C500,
} CecTitles;

#define CEC_PATH_MBOX_LIST 1
#define CEC_PATH_MBOX_INFO 2
#define CEC_PATH_INBOX_INFO 3
#define CEC_PATH_OUTBOX_INFO 4
#define CEC_PATH_OUTBOX_INDEX 5
#define CEC_PATH_INBOX_MSG 6
#define CEC_PATH_OUTBOX_MSG 7
#define CEC_PATH_ROOT_DIR 10
#define CEC_PATH_MBOX_DIR 11
#define CEC_PATH_INBOX_DIR 12
#define CEC_PATH_OUTBOX_DIR 13
#define CECMESSAGE_BOX_ICON 101
#define CECMESSAGE_BOX_TITLE 110

typedef enum {
	CEC_COMMAND_NONE = 0,
	CEC_COMMAND_START = 1,
	CEC_COMMAND_RESET_START = 2,
	CEC_COMMAND_READYSCAN = 3,
	CEC_COMMAND_READYSCANWAIT = 4,
	CEC_COMMAND_STARTSCAN = 5,
	CEC_COMMAND_RESCAN = 6,
	CEC_COMMAND_NDM_RESUME = 7,
	CEC_COMMAND_NDM_SUSPEND = 8,
	CEC_COMMAND_NDM_SUSPEND_IMMEDIATE = 9,
	CEC_COMMAND_STOPWAIT = 0xA,
	CEC_COMMAND_STOP = 0xB,
	CEC_COMMAND_STOP_FORCE = 0xC,
	CEC_COMMAND_STOP_FORCE_WAIT = 0xD,
	CEC_COMMAND_RESET_FILTER = 0xE,
	CEC_COMMAND_DAEMON_STOP = 0xF,
	CEC_COMMAND_DAEMON_START = 0x10,
	CEC_COMMAND_EXIT = 0x11,
	CEC_COMMAND_OVER_BOSS = 0x12,
	CEC_COMMAND_OVER_BOSS_FORCE = 0x13,
	CEC_COMMAND_OVER_BOSS_FORCE_WAIT = 0x14,
	CEC_COMMAND_END = 0x15,
} CecCommand;

typedef enum {
	CEC_STATE_ABBREV_IDLE = 1,
	CEC_STATE_ABBREV_INACTIVE = 2,
	CEC_STATE_ABBREV_SCANNING = 3,
	CEC_STATE_ABBREV_WLREADY = 4,
	CEC_STATE_ABBREV_OTHER = 5,
} CecStateAbbrev;

// seems to be the max number of bytes for transfering stuffs
#define MAX_MESSAGE_SIZE 0x19000
#define MAX_SLOT_SIZE (MAX_MESSAGE_SIZE + 0x14)

typedef u8 CecMessageId[8];

typedef struct SlotMetadata {
	int send_method;
	u32 title_id;
	u32 size;
} SlotMetadata;

Result waitForCecdState(bool start, int command, CecStateAbbrev state);
Result cecdInit(void);
Result cecdGetState(u32* state);
Result cecdGetSystemInfo(u32 destbuf_size, void* destbuf);
Result cecdReadMessage(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id);
Result cecdReadMessageWithHMAC(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id, u8* hmac);
Result cecdWriteMessage(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id);
Result cecdWriteMessageWithHMAC(u32 program_id, bool is_outbox, u32 size, u8* buf, CecMessageId message_id, u8* hmac);
Result cecdStart(CecCommand command);
Result cecdStop(CecCommand command);
Result cecdGetCecdState(CecStateAbbrev* state);
Result cecdGetCecInfoEventHandle(Handle* handle);
Result cecdGetChangeStateEventHandle(Handle* handle);
Result cecdOpenAndWrite(u32 program_id, u32 path_type, u32 size, u8* buf);
Result cecdOpenAndRead(u32 program_id, u32 path_type, u32 size, u8* buf);
Result cecdSprCreate(void);
Result cecdSprInitialise(void);
Result cecdSprGetSlotsMetadata(u32 size, SlotMetadata* buf, u32* slots_total);
Result cecdSprGetSlot(u32 title_id, u32 size, u8* buf);
Result cecdSprSetTitleSent(u32 title_id, bool success);
Result cecdSprFinaliseSend(void);
Result cecdSprStartRecv(void);
Result cecdSprAddSlotsMetadata(u32 size, u8* buf);
Result cecdSprAddSlot(u32 title_id, u32 size, u8* buf);
Result cecdSprFinaliseRecv(void);
Result cecdSprDone(bool success);
Result cecdGetBossUserid(u64* out);
Handle cecdGetServHandle(void);

Result updateStreetpassOutbox(u8* msgbuf);
bool validateStreetpassMessage(u8* msgbuf);
Result addStreetpassMessage(u8* msgbuf);

typedef struct {
	u32 magic; // 0x42504643 CFPB
	u8 unknown[0x14];
	union {
		u8 nonce[0x8];
		struct{
			u32 mii_id;
			u8 top_mac[4];
		};
	};
	u8 ciphertext[0x58];
	u8 mac[0x10];
} CFPB;

typedef struct CecTimestamp {
	u32 year;
	u8 month;
	u8 day;
	u8 weekday;
	u8 hour;
	u8 minute;
	u8 second;
	u16 millisecond;
} CecTimestamp;

typedef struct CecMessageHeader {
	u16 magic; // 0x6060 ``
	u16 padding_sourceident;
	u32 message_size;
	u32 total_header_size;
	u32 body_size;
	u32 title_id;
	u32 title_id2;
	u32 batch_id;
	u32 transfer_id;
	CecMessageId message_id;
	u32 message_version;
	CecMessageId message_id2;
	u8 recipients; // 0x01: everyone, 0x02: friends
	u8 send_method;
	bool unopened;
	bool new_flag;
	u64 sender_id;
	u64 sender_id2;
	CecTimestamp sent;
	CecTimestamp received;
	CecTimestamp created;
	u8 send_count;
	u8 forward_count;
	u16 user_data;
} CecMessageHeader;

typedef struct CecSlotHeader {
	u16 magic; // 0x6161 'aa'
	u16 padding;
	u32 size;
	u32 title_id;
	u32 batch_id;
	u32 message_count;
} CecSlotHeader;

typedef struct CecBoxInfoHeader {
	u16 magic; // 0x6262 'bb'
	u16 padding;
	u32 file_size;
	u32 max_box_size;
	u32 box_size;
	u32 max_num_messages;
	u32 num_messages;
	u32 max_batch_size;
	u32 max_message_size;
} CecBoxInfoHeader;

typedef struct CecBoxInfoFull {
	CecBoxInfoHeader header;
	CecMessageHeader messages[10];
} CecBoxInfoFull;

typedef struct CecMBoxInfoHeader {
	u16 magic; // 0x6363 'cc'
	u16 padding1;
	u32 program_id;
	u32 private_id;
	u8 box_type_flags; // 0x01: normal programs, 0x06: system programs, 0x80: silent notif
	bool enabled; // if we should send/receive things
	u16 padding2;
	u8 hmac_key[32];
	u32 padding3; // mbox info size
	CecTimestamp last_accessed;
	u8 flag_unread; // unread
	u8 flag_new; // new
	u8 flag5;
	u8 flag6;
	CecTimestamp last_received;
	u32 padding5;
	CecTimestamp unknown_time;
} CecMBoxInfoHeader;

typedef struct CecOBIndex {
	u16 magic; // 0x6767 'gg'
	u16 padding;
	u32 num_messages;
	u64 message_ids;
} CecOBIndex;

typedef struct CecMboxListHeader {
	u16 magic; // 0x6868 'hh'
	u16 padding;
	u32 version;
	u32 num_boxes;
	u8 box_names[24][16]; // 12 used, but space for 24
} CecMboxListHeader;

// This is our own custom thing to add capacities
typedef struct CecMboxListHeaderWithCapacities {
	CecMboxListHeader header;
	u32 capacities[24];
} CecMboxListHeaderWithCapacities;

typedef struct CecMessageBodyMarioKart7 {
	u16 message[17];
} CecMessageBodyMarioKart7;

typedef struct CecRegionGameAgeRating {
	u8 japan;
	u8 usa;
	u8 pad1;
	u8 germany;
	u8 europe;
	u8 pad2;
	u8 portugal;
	u8 england;
	u8 australia;
	u8 south_korea;
	u8 taiwan;
	u8 pad3[5];
} CecRegionGameAgeRating;

typedef struct CecApplicationSettings {
	CecRegionGameAgeRating age_rating;
	u32 region_lockout;
	u32 match_maker_id;
	u8 match_maker_bit_id[8];
	u32 flags;
	u16 eula_version;
	u8 pad[2];
	float optimal_anim_frame;
	u32 title_id;
} CecApplicationSettings;

typedef struct CecApplicationTitle {
	u16 short_description[0x80/2];
	u16 long_description[0x100/2];
	u16 publisher[0x80/2];
} CecApplicationTitle;

typedef struct CecMessageMiiPlazaLocation {
	u16 name[0x20];
} CecMessageMiiPlazaLocation;

typedef struct CecMessageMiiPlazaReplyList {
	u32 mii_id;
	u8 mac[6];
} CecMessageMiiPlazaReplyList;

typedef struct CecMessageMiiPlazaReply {
	u16 message[0x11];
} CecMessageMiiPlazaReply;

typedef struct CecMessageBodyMiiPlaza {
	CecApplicationSettings app_settings;
	CecApplicationTitle title[16];
	u8 pad1[0x168C];
	CFPB cfpb;
	u8 pad2[0x1C];
	CecMessageMiiPlazaLocation country[16];
	CecMessageMiiPlazaLocation region[16];
	u8 pad3[16];
	u16 message[17];

	CecMessageMiiPlazaReplyList reply_list[0x10];

	CecMessageMiiPlazaReply reply_msg[0x10];
	CecMessageMiiPlazaReply replied_msg[0x10];
} CecMessageBodyMiiPlaza;

typedef struct CecMessageBodyTomodachiLife {
	u8 pad[52];
	u16 island_name[17];
} CecMessageBodyTomodachiLife;
