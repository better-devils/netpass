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

#include "curl-handler.h"
#include "cecd.h"
#include "api.h"
#include "hmac_sha256/hmac_sha256.h"
#include "utils.h"
#include "debug.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#define MAX_CONNECTIONS 3

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
static u32 *SOC_buffer = NULL;

static Thread curl_multi_thread;
static bool running = false;
static u8 mac[6] = {0};
static char* netpass_id;

#define CURL_HANDLE_STATUS_FREE 0
#define CURL_HANDLE_STATUS_RESERVED 1
#define CURL_HANDLE_STATUS_PENDING 2
#define CURL_HANDLE_STATUS_RUNNING 3
#define CURL_HANDLE_STATUS_DONE 4
#define CURL_HANDLE_STATUS_RESET 5

struct CurlHandle {
	CURL* handle;
	CURLcode result;
	volatile int status;
	char* method;
	char* url;
	char* title_name;
	char* hmac_key;
	int size;
	u8* body;
	Result res;
	CurlReply reply;
	FILE* file_reply;
};

static struct CurlHandle handles[MAX_CONNECTIONS] = {0};

Result getMac(u8 mac[6]) {
	Result res = 0;
	Handle handle;
	res = srvGetServiceHandle(&handle, "nwm::SOC");
	if (R_FAILED(res)) return res;

	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(8, 1, 0);
	cmdbuf[1] = 6;

	u32 saved_threadstorage[2];

	u32 *staticbufs = getThreadStaticBuffers();
	saved_threadstorage[0] = staticbufs[0];
	saved_threadstorage[1] = staticbufs[1];
	staticbufs[0] = IPC_Desc_StaticBuffer(6, 0);
	staticbufs[1] = (u32)mac;
	res = svcSendSyncRequest(handle);
	staticbufs[0] = saved_threadstorage[0];
	staticbufs[1] = saved_threadstorage[1];
	if (R_FAILED(res)) return res;
	res = (Result)cmdbuf[1];
	memcpy(mac, (u8*)cmdbuf[3], 6);
	return res;
}

size_t curlWrite(void *data, size_t size, size_t nmemb, void* ptr) {
	CurlReply* r = (CurlReply*)ptr;
	size_t new_len = r->len + size*nmemb;
	if (new_len > MAX_SLOT_SIZE) {
		return 0;
	}
	memcpy(r->ptr + r->len, data, size*nmemb);
	if (new_len + 1 < MAX_SLOT_SIZE) {
		r->ptr[new_len] = '\0';
	}
	r->len = new_len;
	return size*nmemb;
}

size_t curlHeader(void *data, size_t size, size_t nmemb, void* ptr) {
	char buf[size*nmemb + 1];
	memcpy(buf, data, size*nmemb);
	buf[size*nmemb] = '\0';
	static const char header_name[] = "3ds-netpass-msg: ";
	if (strncmp(header_name, buf, strlen(header_name)) == 0) {
		printf("%s\n", buf + strlen(header_name));
	}
	return size*nmemb;
}

void curlFreeHandler(int offset) {
	handles[offset].status = CURL_HANDLE_STATUS_RESET;
}

Result httpRequest(char* method, char* url, int size, u8* body, CurlReply** reply, char* title_name, char* hmac_key) {
	Result res = 0;
	int curl_handle_slot = 0;
	bool found_handle_slot = false;
	for (; curl_handle_slot < MAX_CONNECTIONS; curl_handle_slot++) {
		if (handles[curl_handle_slot].status == CURL_HANDLE_STATUS_FREE) {
			handles[curl_handle_slot].status = CURL_HANDLE_STATUS_RESERVED;
			found_handle_slot = true;
			break;
		}
	}
	if (!found_handle_slot) {
		// TODO: dunno, wait or something?
		return -1;
	}

	FILE* file = 0;
	if ((u32)reply == 1) {
		// we have a file reply
		file = fopen(title_name, "wb");
		if (!file) {
			_e(-1);
			return -2;
		}
	}
	handles[curl_handle_slot].file_reply = file;

	handles[curl_handle_slot].method = method;
	handles[curl_handle_slot].url = url;
	handles[curl_handle_slot].size = size;
	handles[curl_handle_slot].body = body;
	handles[curl_handle_slot].title_name = title_name;
	handles[curl_handle_slot].hmac_key = hmac_key;
	if (handles[curl_handle_slot].file_reply) {
		handles[curl_handle_slot].title_name = 0;
		handles[curl_handle_slot].hmac_key = 0;
	}
	handles[curl_handle_slot].status = CURL_HANDLE_STATUS_PENDING;
	// request is being sent, let's wait until it is back
	
	while (handles[curl_handle_slot].status != CURL_HANDLE_STATUS_DONE) {
		//printf("%d", handles[curl_handle_slot].status);
		svcSleepThread((u64)1000000 * 100);
	}

	res = handles[curl_handle_slot].res;
	if (reply && (u32)reply != 1) {
		*reply = &handles[curl_handle_slot].reply;
	} else {
		curlFreeHandler(curl_handle_slot);
	}
	if (file) fclose(file);
	return res;
}

static CURLM* curl_multi_handle;

void curl_multi_loop_request_finish(int i) {
	struct CurlHandle* h = &handles[i];
	h->res = h->result;
	if (h->res != CURLE_OK) {
		h->res = -h->res;
		goto cleanup;
	}
	long http_code = 0;
	curl_easy_getinfo(h->handle, CURLINFO_RESPONSE_CODE, &http_code);
	if (!(http_code >= 200 && http_code < 300)) {
		h->res = -http_code;
		goto cleanup;
	}
	h->res = http_code;
cleanup:
	curl_multi_remove_handle(curl_multi_handle, h->handle);
	curl_easy_cleanup(h->handle);
	h->handle = 0;
	h->status = CURL_HANDLE_STATUS_DONE;
}

u8* getMacBuf(void) {
	return mac;
}


void getMacStr(char value[13]) {
	for (int i = 0; i < 6; i++) {
		value += sprintf(value, "%02X", mac[i]);
	}
}

void curl_multi_loop_request_setup(int i) {
	struct CurlHandle* h = &handles[i];
	h->handle = curl_easy_init();
	if (!h->handle) {
		h->res = -1;
		h->status = CURL_HANDLE_STATUS_DONE;
		return;
	}
	struct curl_slist* headers = NULL;

	// add mac header
	char header_mac[25];
	char header_mac_value[13];
	getMacStr(header_mac_value);
	snprintf(header_mac, sizeof(header_mac), "3ds-mac: %s", header_mac_value);
	headers = curl_slist_append(headers, header_mac);
	
	// add nid header
	char header_netpass_id[100];
	snprintf(header_netpass_id, 100, "3ds-nid: %s", netpass_id);
	headers = curl_slist_append(headers, header_netpass_id);
	
	// add version header
	char header_netpass_version[100];
#ifdef _VERSION_GIT_SHA_
	// cppcheck-suppress invalidPrintfArgType_s
	snprintf(header_netpass_version, sizeof(header_netpass_version),
			 "3ds-netpass-version: v%d.%d.%d-%s",
			 _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_, _VERSION_GIT_SHA_);
#else
	snprintf(header_netpass_version, sizeof(header_netpass_version),
			 "3ds-netpass-version: v%d.%d.%d",
			 _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
#endif
	DEBUG_PRINTF("header_netpass_version: %s\n", header_netpass_version);
	headers = curl_slist_append(headers, header_netpass_version);
	
	// add time header
	char header_time[100];
	time_t unixTime = time(NULL);
	struct tm* ts = gmtime((const time_t *)&unixTime);
	snprintf(header_time, 100, "3ds-time: %02i:%02i:%02i", ts->tm_hour, ts->tm_min, ts->tm_sec);
	headers = curl_slist_append(headers, header_time);
	
	FriendKey friend_key;
	Result res = FRD_GetMyFriendKey(&friend_key);
	if (R_SUCCEEDED(res)) {
		u64 fc;
		res = FRD_PrincipalIdToFriendCode(friend_key.principalId, &fc);
		if (R_SUCCEEDED(res)) {
			char header_fc[100];
			snprintf(header_fc, 100, "3ds-fc: %016llX", fc);
			headers = curl_slist_append(headers, header_fc);
			u64 fc_seed;
			res = CFGI_GetLocalFriendCodeSeed(&fc_seed);
			if (R_SUCCEEDED(res)) {
				char header_friend_key[100];
				snprintf(header_friend_key, 100, "3ds-friend-code: %016llX", (fc ^ fc_seed) & 0x0000ffffffffffffll);
				headers = curl_slist_append(headers, header_friend_key);
			}
		}
	}
	if (h->title_name && !h->file_reply) {
		char header_title_name[225];
		snprintf(header_title_name, 225, "3ds-title-name: %s", h->title_name);
		headers = curl_slist_append(headers, header_title_name);
	}
	if (h->hmac_key && !h->file_reply) {
		char header_hmac_key[255];
		snprintf(header_hmac_key, 255, "3ds-hmac-key: %s", h->hmac_key);
		headers = curl_slist_append(headers, header_hmac_key);
	}

	if (h->body) {
		curl_easy_setopt(h->handle, CURLOPT_POSTFIELDS, h->body);
		headers = curl_slist_append(headers, "Content-Type: application/binary");
		curl_easy_setopt(h->handle, CURLOPT_POSTFIELDSIZE, h->size);
	}

	// set some options
	curl_easy_setopt(h->handle, CURLOPT_URL, h->url);
	curl_easy_setopt(h->handle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(h->handle, CURLOPT_USERAGENT, "3ds");
	curl_easy_setopt(h->handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(h->handle, CURLOPT_MAXREDIRS, 50);
	curl_easy_setopt(h->handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
	curl_easy_setopt(h->handle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(h->handle, CURLOPT_CUSTOMREQUEST, h->method);
	curl_easy_setopt(h->handle, CURLOPT_TIMEOUT, 120);
	curl_easy_setopt(h->handle, CURLOPT_SERVER_RESPONSE_TIMEOUT, 10);
	curl_easy_setopt(h->handle, CURLOPT_CONNECTTIMEOUT, 20);
	curl_easy_setopt(h->handle, CURLOPT_NOSIGNAL, 0);
	curl_easy_setopt(h->handle, CURLOPT_SSL_VERIFYPEER, 1);
	curl_easy_setopt(h->handle, CURLOPT_CAINFO, "romfs:/certs.pem");
	curl_easy_setopt(h->handle, CURLOPT_HEADERFUNCTION, curlHeader);
	curl_easy_setopt(h->handle, CURLOPT_HEADERDATA, NULL);

	if (h->file_reply) {
		curl_easy_setopt(h->handle, CURLOPT_WRITEFUNCTION, fwrite);
		curl_easy_setopt(h->handle, CURLOPT_WRITEDATA, h->file_reply);
	} else {
		curl_easy_setopt(h->handle, CURLOPT_WRITEFUNCTION, curlWrite);
		h->reply.len = 0;
		h->reply.offset = i;
		curl_easy_setopt(h->handle, CURLOPT_WRITEDATA, &h->reply);
	}

	h->status = CURL_HANDLE_STATUS_RUNNING;
	curl_multi_add_handle(curl_multi_handle, h->handle);
}

void curl_multi_loop(void* p) {
	curl_multi_handle = curl_multi_init();
	running = true;
	int openHandles = 0;
	do {
		CURLMcode mc = curl_multi_perform(curl_multi_handle, &openHandles);
		if (mc != CURLM_OK) {
			printf("ERROR curl multi fail: %u\n", mc);
			return;
		}
		CURLMsg* msg;
		int msgsLeft;
		while ((msg = curl_multi_info_read(curl_multi_handle, &msgsLeft))) {
			if (msg->msg == CURLMSG_DONE) {
				for (int i = 0; i < MAX_CONNECTIONS; i++) {
					if (handles[i].handle == msg->easy_handle) {
						handles[i].result = msg->data.result;
						curl_multi_loop_request_finish(i);
						break;
					}
				}
			}
		}
		if (!openHandles) {
			svcSleepThread((u64)1000000 * 100);
		} else {
			svcSleepThread(1000000);
		}
		for (int i = 0; i < MAX_CONNECTIONS; i++) {
			if (handles[i].status == CURL_HANDLE_STATUS_RESET) {
				handles[i].handle = 0;
				handles[i].result = 0;
				handles[i].status = CURL_HANDLE_STATUS_FREE;
			}
			if (handles[i].status == CURL_HANDLE_STATUS_PENDING) {
				curl_multi_loop_request_setup(i);
			}
		}
	} while (running);
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (handles[i].handle) {
			curl_multi_remove_handle(curl_multi_handle, handles[i].handle);
			curl_easy_cleanup(handles[i].handle);
		}
	}
	curl_multi_cleanup(curl_multi_handle);
}

Result curlInit(void) {
	Result res;
	// ok, we have to init this first
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	if (!SOC_buffer) return -1;
	res = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (R_FAILED(res)) return res;
	curl_global_init(CURL_GLOBAL_ALL);

	u32 device_id;
	res = AM_GetDeviceId(&device_id);
	if (R_FAILED(res)) return res;
	res = getMac(mac);
	if (R_FAILED(res)) return res;

	u8 netpass_id_buf[32];
	hmac_sha256(&device_id, 4, mac, 6, netpass_id_buf, 32);
	netpass_id = b64encode(netpass_id_buf, 32);

	curl_multi_thread = threadCreate(curl_multi_loop, NULL, 8*1024, main_thread_prio()-1, -2, false);

	return res;
}

void curlExit(void) {
	running = false;
	if (curl_multi_thread) {
		// Wait for the thread to exit (wait 10 sec)
		const s64 timeout10sec = 10 * 1000 * 1000 * 1000LL;
		threadJoin(curl_multi_thread, timeout10sec);
		threadFree(curl_multi_thread);
	}
	free(netpass_id);
	curl_global_cleanup();
	socExit();
}
