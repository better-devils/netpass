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

void qr_buffer_from_quirc_data(QrBuffer* buffer, struct quirc_data* data) {
	buffer->start = data->payload;
	buffer->size = data->payload_len;
	buffer->cur = buffer->start;
	buffer->end = buffer->start + buffer->size;
}

u32 qr_read_u32(QrBuffer* buffer) {
	u32 ret = *(u32*)buffer->cur;
	if (!qr_buffer_consume(buffer, sizeof(u32))) return 0;
	return ret;
}

u32 qr_read_string(QrBuffer* buffer, char* string, u32 length) {
	u32 string_length = qr_read_u32(buffer);
	u8* cur = buffer->cur;
	if (!qr_buffer_consume(buffer, string_length)) return 0;
	u32 copy_length = string_length < length ? string_length : length;
	strncpy(string, (char*)cur, copy_length);
	string[copy_length - 1] = 0;
	return copy_length;
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
