/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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

#include "integration.h"
#include "api.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

static IntegrationList* g_list = 0;

static Result lazy_init(void) {
	Result res;
	CurlReply* reply;
	char url[80];
	snprintf(url, 80, "%s/integration", BASE_URL);
	res = _e(httpRequest("GET", url, 0, 0, &reply, 0));
	if (R_FAILED(res)) goto cleanup;
	int http_code = res;
	IntegrationListHeader* list_header = (IntegrationListHeader*)reply->ptr;
	if (!IS_HTTP_SUCCESS(http_code) || list_header->magic != 0x4C49504E || list_header->version != 1) {
		res = _e(ERROR_BAD_INTEGRATION_LIST);
		goto cleanup;
	}
	g_list = malloc(list_header->size);
	if (!g_list) {
		res = _e(ERROR_OUT_OF_MEMORY);
		goto cleanup;
	}
	memcpy(g_list, reply->ptr, list_header->size);
cleanup:
	curlFreeHandler(reply->offset);
	return res;
}

IntegrationList* get_integration_list(void) {
	if (!g_list) {
		Result res = _e(lazy_init());
		if (R_FAILED(res)) {
			return 0;
		}
	}
	return g_list;
}

Result toggle_integration(u32 id) {
	Result res = 0;
	if (!g_list) {
		res = _e(lazy_init());
	}
	if (R_FAILED(res)) return res;
	int index = -1;
	for (int i = 0; i < g_list->header.count; i++) {
		if (g_list->entries[i].id == id) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		res = -1;
		return res;
	}
	char url[80];
	snprintf(url, 80, "%s/integration/%ld", BASE_URL, id);
	char* method = g_list->entries[index].enabled ? "DELETE" : "PUT";
	res = _e(httpRequest(method, url, 0, 0, 0, 0));
	if (R_FAILED(res)) return res;
	g_list->entries[index].enabled = !g_list->entries[index].enabled;
	return res;
}

void integrationExit(void) {
	if (g_list) {
		free(g_list);
		g_list = 0;
	}
}
