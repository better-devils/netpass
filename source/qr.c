/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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

#include "qr.h"
#include <string.h>
#include "api.h"
#include "cecd.h"
#include "curl-handler.h"
#include "utils.h"

bool qr_buffer_consume(QrBuffer* buffer, u32 length) {
	if (buffer->cur + length > buffer->end) return false;
	buffer->cur += length;
	return true;
}

void qr_buffer_new(QrBuffer* buffer, u8* bytes, u32 size) {
	buffer->start = bytes;
	buffer->size = size;
	buffer->cur = buffer->start;
	buffer->end = buffer->start + buffer->size;
}

void qr_buffer_from_quirc_data(QrBuffer* buffer, struct quirc_data* data) {
	qr_buffer_new(buffer, data->payload, data->payload_len);
}

u32 qr_read_u32(QrBuffer* buffer) {
	u32 ret = *(u32*)buffer->cur;
	if (!qr_buffer_consume(buffer, sizeof(u32))) return 0;
	return ret;
}

u32 qr_peek_u32(QrBuffer* buffer) {
	return *(u32*)buffer->cur;
}

u8 qr_read_u8(QrBuffer* buffer) {
	u8 ret = *(u8*)buffer->cur;
	if (!qr_buffer_consume(buffer, sizeof(u8))) return 0;
	return ret;
}

bool qr_read_bool(QrBuffer* buffer) {
	return qr_read_u8(buffer) != 0;
}

u32 qr_read_object(QrBuffer* buffer, void* buf, u32 length) {
	u8* cur = buffer->cur;
	if (!length || !qr_buffer_consume(buffer, length)) return 0;
	memcpy(buf, cur, length);
	return length;
}

u32 qr_read_string(QrBuffer* buffer, char* string, u32 length) {
	u32 string_length = qr_read_u32(buffer);
	u8* cur = buffer->cur;
	if (!string_length || !qr_buffer_consume(buffer, string_length)) return 0;
	u32 copy_length = string_length < length ? string_length : length;
	strncpy(string, (char*)cur, copy_length);
	string[copy_length - 1] = 0;
	return copy_length;
}

u32 qr_read_align(QrBuffer* buffer, u32 align) {
	u8 num = align - ((buffer->cur - buffer->start) % align);
	if (num != align && !qr_buffer_consume(buffer, num)) return 0;
	return num;
}

bool qr_buf_equal(QrBuffer* buffer, u8* buf, u32 len) {
	u8* cur = buffer->cur;
	if (!qr_buffer_consume(buffer, len)) return false;
	return memcmp(cur, buf, len) == 0;
}

Result qr_verify(QrBuffer* buffer) {
	char token[300];
	Result res = 0;
	if (qr_read_string(buffer, token, 300) == 0) {
		return ERROR_MISSING_TOKEN;
	}
	char url[80];
	snprintf(url, 80, "%s/verify", BASE_URL);
	res = httpRequest("POST", url, strlen(token) + 1, (u8*)token, 0, 0, 0);
	if (R_FAILED(res)) return res;
	int http_code = res;
	if (!IS_HTTP_SUCCESS(http_code)) return -res;
	return res;
}

Result qr_dl_pass(QrBuffer* buffer) {
	char url[300];
	Result res = 0;
	if (qr_read_string(buffer, url, 300) == 0) {
		return ERROR_MISSING_PASS_URL;
	}
	CurlReply* reply;
	res = httpRequest("GET", url, 0, 0, &reply, 0, 0);
	if (R_FAILED(res)) goto fail;
	int http_code = res;
	if (!IS_HTTP_SUCCESS(http_code)) {
		res = -res;
		goto fail;
	}
	if (reply->len < sizeof(CecMessageHeader)) {
		res = ERROR_INVALID_MESSAGE;
		goto fail;
	}
	res = addStreetpassMessage(reply->ptr);
fail:
	curlFreeHandler(reply->offset);
	return res;
}
