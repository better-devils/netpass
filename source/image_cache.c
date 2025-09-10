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

#include "curl-handler.h"
#define IMAGE_CACHE_DIR "sdmc:/config/netpass/cache/images/"

#include "image_cache.h"
#include "api.h"
#include "utils.h"
#include <unistd.h>

Result cache_current_location_image(void) {
	Result res = 0;
	if (!location.have_image) return res;
	
	char filename[100];
	char hash[0x20*2 + 1];
	for (int i = 0; i < 0x20; i++) {
		snprintf(hash + (i*2), 3, "%02X", location.image_hash[i]);
	}
	snprintf(filename, 100, "%s%s.png", IMAGE_CACHE_DIR, hash);
	if (access(filename, F_OK) == 0) return res;
	
	// ok, time to download the image
	mkdir_p(IMAGE_CACHE_DIR);
	
	char url[100];
	printf("Downloading new location image %s.png...", hash);
	snprintf(url, 100, "%s/location/current/image", BASE_URL);
	res = httpRequest("GET", url, 0, 0, (void*)1, filename, 0);
	
	return res;
}

bool get_current_location_image(C2D_Image* img) {
	char filename[100];
	char hash[0x20*2 + 1];
	for (int i = 0; i < 0x20; i++) {
		snprintf(hash + (i*2), 3, "%02X", location.image_hash[i]);
	}
	snprintf(filename, 100, "%s%s.png", IMAGE_CACHE_DIR, hash);
	return loadFilePng(img, filename);
}
