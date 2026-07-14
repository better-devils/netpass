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

#include "curl-handler.h"
#include "cecd.h"
#include "api.h"
#include "hmac_sha256/hmac_sha256.h"
#include "log.h"
#include "utils.h"
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
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
static char nid_password[16] = {0};
static bool sent_extra_ident = false;

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
	const char* method;
	const char* url;
	int size;
	u8* body;
	Result res;
	CurlReply reply;
	FILE* file_reply;
};

static struct CurlHandle handles[MAX_CONNECTIONS] = {0};

static Result getEffectiveMac(u8 mac[6]) {
	Result res = 0;
	FILE* f = fopen(PATH_MAC, "r");
	if (f) {
		if (fread(mac, 6, 1, f) != 1) res = _e_errno();
		fclose(f);
		return res;
	}
	res = getMac(mac);
	if (R_FAILED(res)) return res;
	
	mkdir_p(PATH_MAC);
	f = fopen(PATH_MAC, "w");
	if (!f) {
		res = _e_errno();
		return res;
	}
	if (fwrite(mac, 6, 1, f) != 1) res = _e_errno();
	fclose(f);
	if (R_FAILED(res)) return res;
	
	mkdir_p(PATH_MAC_BAK);
	f = fopen(PATH_MAC_BAK, "w");
	if (!f) {
		res = _e_errno();
		return res;
	}
	if (fwrite(mac, 6, 1, f) != 1) res = _e_errno();
	fclose(f);
	return res;
}

static size_t curlWrite(void *data, size_t size, size_t nmemb, void* ptr) {
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

static int xferinfo_callback(void *ptr, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	struct CurlHandle *h = ptr;
	h->reply.dltotal = dltotal;
	h->reply.dlnow = dlnow;
	return 0;
}

static size_t curlHeader(void *data, size_t size, size_t nmemb, void* ptr) {
	struct CurlHandle *h = ptr;
	char header_name[size*nmemb + 1];
	memcpy(header_name, data, size*nmemb);
	header_name[size*nmemb] = '\0';
	
	char* header_value = strchr(header_name, ':');
	if (!header_value) return size*nmemb;
	*header_value = 0;
	header_value++;
	if (!*header_value) return size*nmemb;
	header_value++;
	
	int header_name_len = strlen(header_name);
	for (int i = 0; i < header_name_len; i++) header_name[i] = tolower(header_name[i]);
	
	if (strcmp("date", header_name) == 0) {
		h->reply.date = curl_getdate(header_value, NULL);
	}
	if (strcmp("3ds-netpass-msg", header_name) == 0) {
		logln(INFO, "%s", header_value);
	}
	if (strcmp("x-spr-slot00-result", header_name) == 0) {
		char* ptr = header_value;
		ptr = strchr(ptr, ',');
		if (ptr) {
			ptr++;
			char* end = strchr(ptr, ',');
			if (end) {
				*end = 0;
				h->reply.header = atoi(ptr);
			}
		}
	}
	return size*nmemb;
}

void curlFreeHandler(int offset) {
	handles[offset].status = CURL_HANDLE_STATUS_RESET;
}

void resetCurlRegistrationCache(void) {
	memset(nid_password, 0, sizeof(nid_password));
	sent_extra_ident = false;
}

Result httpRequest(const char* method, const char* url, int size, u8* body, CurlReply** reply, const char* filename) {
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
		return ERROR_CURL_NO_FREE_HANDLE;
	}

	FILE* file = 0;
	if (filename) {
		// we have a file reply
		file = fopen(filename, "wb");
		if (!file) {
			_e_errno();
			return ERROR_ERRNO;
		}
	}
	handles[curl_handle_slot].file_reply = file;

	handles[curl_handle_slot].method = method;
	handles[curl_handle_slot].url = url;
	handles[curl_handle_slot].size = size;
	handles[curl_handle_slot].body = body;
	
	if (reply) {
		*reply = &handles[curl_handle_slot].reply;
	}
	handles[curl_handle_slot].reply.dltotal = 0;
	handles[curl_handle_slot].reply.dlnow = 0;
	handles[curl_handle_slot].reply.header = 0xFFFFFFFF;
	
	handles[curl_handle_slot].status = CURL_HANDLE_STATUS_PENDING;
	// request is being sent, let's wait until it is back
	
	while (handles[curl_handle_slot].status != CURL_HANDLE_STATUS_DONE) {
		//printf("%d", handles[curl_handle_slot].status);
		svcSleepThread((u64)1000000 * 100);
	}

	res = handles[curl_handle_slot].res;
	if (!reply) {
		curlFreeHandler(curl_handle_slot);
	}
	if (file) fclose(file);
	return res;
}

static CURLM* curl_multi_handle;

static void curl_multi_loop_request_finish(int i) {
	struct CurlHandle* h = &handles[i];
	h->res = h->result;
	if (h->res != CURLE_OK) {
		h->res = -h->res;
		goto cleanup;
	}
	long http_code = 0;
	curl_easy_getinfo(h->handle, CURLINFO_RESPONSE_CODE, &http_code);
	if (!IS_HTTP_SUCCESS(http_code)) {
		h->res = -http_code;
		goto cleanup;
	}
	h->res = http_code;
cleanup:
	curl_multi_remove_handle(curl_multi_handle, h->handle);
	if (h->handle) {
		curl_easy_cleanup(h->handle);
	} else {
		logln(WARN, "Tried to free already freed handle with offset %d", i);
	}
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

static Result ACU_GetProxyAuthType(u8* auth_type) {
	Result ret = 0;
	u32* cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x37, 0, 0);
	
	if (R_FAILED(ret = svcSendSyncRequest(*acGetSessionHandle()))) return ret;
	*auth_type = (u8)cmdbuf[2];
	return (Result)cmdbuf[1];
}

static Result curl_add_proxy(CURL* handle) {
	Result res = 0;
	bool enable;
	res = ACU_GetProxyEnable(&enable);
	if (R_FAILED(res) || !enable) {
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1);
		return res;
	}
	{
		char host[0x100] = {0};
		u16 port;
		res = ACU_GetProxyHost(host);
		if (R_FAILED(res)) return res;
		res = ACU_GetProxyPort(&port);
		if (R_FAILED(res)) return res;
		curl_easy_setopt(handle, CURLOPT_PROXY, host);
		curl_easy_setopt(handle, CURLOPT_PROXYPORT, port);
	}
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(handle, CURLOPT_PROXY_SSL_VERIFYPEER, 0);
	u8 auth_type = 0;
	ACU_GetProxyAuthType(&auth_type);
	switch (auth_type) {
		case 0:
			break;
		case 1: { // simple auth
			char username[0x20] = {0};
			char password[0x20] = {0};
			res = ACU_GetProxyUserName(username);
			if (R_FAILED(res)) return res;
			res = ACU_GetProxyPassword(password);
			if (R_FAILED(res)) return res;
			curl_easy_setopt(handle, CURLOPT_PROXYUSERNAME, username);
			curl_easy_setopt(handle, CURLOPT_PROXYPASSWORD, password);
		}
	}
	
	return res;
}

static void curl_multi_loop_request_setup(int i) {
	struct CurlHandle* h = &handles[i];
	h->handle = curl_easy_init();
	if (!h->handle) {
		h->res = ERROR_CURL_NO_FREE_HANDLE;
		h->status = CURL_HANDLE_STATUS_DONE;
		return;
	}
	curl_add_proxy(h->handle);
	struct curl_slist* headers = NULL;
	
	if (!nid_password[0]) {
		// attempt to load the nid password
		FILE* f = fopen(PATH_NID_PWD, "r");
		if (f) {
			if (fread(nid_password, sizeof(nid_password), 1, f) != 1) {
				nid_password[0] = 0;
			}
			fclose(f);
		}
	}

	// add mac header
	{
		char header_mac[25];
		char header_mac_value[13];
		getMacStr(header_mac_value);
		snprintf(header_mac, sizeof(header_mac), "3ds-mac: %s", header_mac_value);
		headers = curl_slist_append(headers, header_mac);
	}
	
	// add nid header
	{
		char header_netpass_id[100];
		snprintf(header_netpass_id, 100, "3ds-nid: %s", netpass_id);
		headers = curl_slist_append(headers, header_netpass_id);
	}
	
	// add version header
	{
		char header_netpass_version[100];
#ifdef _VERSION_GIT_SHA_
		// cppcheck-suppress invalidPrintfArgType_s
		snprintf(header_netpass_version, sizeof(header_netpass_version),
				 "3ds-netpass-version: v%d.%d.%d+%s",
				 _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_, _VERSION_GIT_SHA_);
#else
		snprintf(header_netpass_version, sizeof(header_netpass_version),
				 "3ds-netpass-version: v%d.%d.%d",
				 _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
#endif
		logln(DEBUG, "header_netpass_version: %s\n", header_netpass_version);
		headers = curl_slist_append(headers, header_netpass_version);
	}

	// add time header
	{
		char header_time[100];
		time_t unixTime = time(NULL);
		struct tm* ts = gmtime((const time_t *)&unixTime);
		snprintf(header_time, sizeof(header_time), "3ds-time: %02i:%02i:%02i", ts->tm_hour, ts->tm_min, ts->tm_sec);
		headers = curl_slist_append(headers, header_time);
	}
	
	// add nid token header
	if (nid_password[0] != 0) {
		char header_nid_token[100];
		char otp_str[11];
		time_t unixTime = time(NULL);
		struct tm* ts = gmtime((const time_t *)&unixTime);
		snprintf(otp_str, sizeof(otp_str), "%04i-%02i-%02i", ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday);
		
		u8 hash[32];
		hmac_sha256(nid_password, 16, otp_str, strlen(otp_str), hash, 32);
		u32 offset = snprintf(header_nid_token, sizeof(header_nid_token), "3ds-nid-token: ");
		char* s = header_nid_token + offset;
		for (int i = 0; i < 32; i++) {
			sprintf(s, "%02x", hash[i]);
			s += 2;
		}
		headers = curl_slist_append(headers, header_nid_token);
	}
	
	if (!sent_extra_ident) {
		// add extra ident headers
		Result res;
		{
			FriendKey friend_key;
			res = FRD_GetMyFriendKey(&friend_key);
			if (R_SUCCEEDED(res)) {
				char header_fc[100];
				snprintf(header_fc, sizeof(header_fc), "3ds-fc: %016llX", friend_key.localFriendCode);
				headers = curl_slist_append(headers, header_fc);
				
				char header_pid[100];
				snprintf(header_pid, sizeof(header_pid), "3ds-pid: %ld", friend_key.principalId);
				headers = curl_slist_append(headers, header_pid);
			}
		}
		{
			u64 seed;
			res = CFGI_GetLocalFriendCodeSeed(&seed);
			if (R_SUCCEEDED(res)) {
				char header_lfcs[100];
				snprintf(header_lfcs, sizeof(header_lfcs), "3ds-lfcs: %016llX", seed);
				headers = curl_slist_append(headers, header_lfcs);
			}
		}
		{
			u64 boss_userid;
			res = cecdGetBossUserid(&boss_userid);
			if (R_SUCCEEDED(res)) {
				char header_bossuid[100];
				snprintf(header_bossuid, sizeof(header_bossuid), "3ds-boss-userid: %016llX", boss_userid);
				headers = curl_slist_append(headers, header_bossuid);
			}
		}
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
	if (!h->file_reply) {
		curl_easy_setopt(h->handle, CURLOPT_TIMEOUT, 40);
	}
	curl_easy_setopt(h->handle, CURLOPT_SERVER_RESPONSE_TIMEOUT, 10);
	curl_easy_setopt(h->handle, CURLOPT_CONNECTTIMEOUT, 20);
	curl_easy_setopt(h->handle, CURLOPT_NOSIGNAL, 0);
	curl_easy_setopt(h->handle, CURLOPT_CAINFO, "romfs:/certs.pem");
	curl_easy_setopt(h->handle, CURLOPT_HEADERFUNCTION, curlHeader);
	curl_easy_setopt(h->handle, CURLOPT_HEADERDATA, h);

	if (h->file_reply) {
		curl_easy_setopt(h->handle, CURLOPT_WRITEFUNCTION, fwrite);
		curl_easy_setopt(h->handle, CURLOPT_WRITEDATA, h->file_reply);
	} else {
		curl_easy_setopt(h->handle, CURLOPT_WRITEFUNCTION, curlWrite);
		h->reply.len = 0;
		h->reply.offset = i;
		curl_easy_setopt(h->handle, CURLOPT_WRITEDATA, &h->reply);
	}
	
	// set up progress
	curl_easy_setopt(h->handle, CURLOPT_XFERINFODATA, h);
    curl_easy_setopt(h->handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(h->handle, CURLOPT_XFERINFOFUNCTION, xferinfo_callback);

	h->status = CURL_HANDLE_STATUS_RUNNING;
	curl_multi_add_handle(curl_multi_handle, h->handle);
}

static void curl_multi_loop(void* p) {
	curl_multi_handle = curl_multi_init();
	running = true;
	int openHandles = 0;
	do {
		CURLMcode mc = curl_multi_perform(curl_multi_handle, &openHandles);
		if (mc != CURLM_OK) {
			logln(ERROR, "curl multi fail: %u", mc);
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
				if (R_SUCCEEDED(handles[i].result) && nid_password[0]) {
					sent_extra_ident = true;
				}
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
	if (!SOC_buffer) return ERROR_OUT_OF_MEMORY;
	res = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (R_FAILED(res)) return res;
	curl_global_init(CURL_GLOBAL_ALL);

	u32 device_id;
	res = AM_GetDeviceId(0, &device_id);
	if (R_FAILED(res)) return res;
	res = getEffectiveMac(mac);
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
