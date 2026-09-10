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

#pragma once

#include <3ds.h>
#include <curl/curl.h>
#include <libeedle.h>

typedef struct {
	u8 ptr[MAX_SLOT_SIZE];
	size_t len;
	time_t date;
	int offset;
	curl_off_t dltotal;
	curl_off_t dlnow;
	int header;
} CurlReply;

#define IS_HTTP_SUCCESS(x) ((x) >= 200 && (x) < 300)

void initCurlReply(CurlReply* r, size_t size);
void deinitCurlReply(CurlReply* r);
Result curlInit(void);
void curlExit(void);
void curlFreeHandler(int offset);
Result httpRequest(const char* method, const char* url, int size, u8* body, CurlReply** reply, const char* filename);
u8* getMacBuf(void);
void getMacStr(char value[13]);
void resetCurlRegistrationCache(void);
