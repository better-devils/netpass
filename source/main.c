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

#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "debug.h"
#include "scene.h"
#include "api.h"
#include "cecd.h"
#include "curl-handler.h"
#include "config.h"
#include "report.h"
#include "music.h"
#include "integration.h"

int main() {
	osSetSpeedupEnable(true); // enable speedup on N3DS

	gfxInitDefault();
	cfguInit();
	amInit();
	nsInit();
	aptInit();
	frdInit(false);
	fsInit();
	consoleInit(GFX_BOTTOM, NULL);
	printf("Starting NetPass v%d.%d.%d", _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
#ifdef _VERSION_GIT_SHA_
	// cppcheck-suppress invalidPrintfArgType_s
	printf("+%s", _VERSION_GIT_SHA_);
#endif
	printf("\n");
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	romfsInit();
	init_main_thread_prio();

	DEBUG_PRINTF("DEBUG ON\n");

	cecdInit();
	Result res = curlInit();
	if (R_FAILED(res)) {
		DEBUG_PRINTF("Curl initialization failed\n");
	}
	srand(time(NULL));

	configInit(); // must be after cecdInit()
	stringsInit(); // must be after configInit()
	musicInit(); // must be after romfsInit()

	// mount sharedextdata_b so that we can read it later, for e.g. playcoins
	{
		u32 extdata_lowpathdata[3] = {0};
		extdata_lowpathdata[0] = MEDIATYPE_NAND;
		extdata_lowpathdata[1] = 0xf000000b;
		FS_Path extdata_path = {
			type: PATH_BINARY,
			size: 0xC,
			data: (u8*)extdata_lowpathdata,
		};
		archiveMount(ARCHIVE_SHARED_EXTDATA, extdata_path, "sharedextdata_b");
		FSUSER_OpenArchive(&sharedextdata_b, ARCHIVE_SHARED_EXTDATA, extdata_path);
	}
	
	playMusic("home"); // start the default music

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	Scene* scene;
	{
		OS_VersionBin ver;
		Result res = get_os_version(&ver);
		if (R_FAILED(res)) {
			printf("osGetSystemVersionData res: %08lX\n", res);
			
			printf("Detected system version (cver): %d.%d.%d%c\n", ver.mainver, ver.minor, ver.build, ver.region);
			u8 region;
			res = CFGU_SecureInfoGetRegion(&region);
			printf("Get region (%08lX): %d\n", res, region);
		}

	
		if (SYSTEM_VERSION(ver.mainver, ver.minor, ver.build) < SYSTEM_VERSION(11, 15, 0)) {
			scene = getBadOsVersionScene();
		} else {
			// somehow cpp check fails with this lambda for the ptr->int return type check
			// as it does not even compile if we were to cast the returns to ints, this is clearly a cppcheck bug
			// cppcheck-suppress CastAddressToIntegerAtReturn
			scene = getLoadingScene(getSwitchScene(lambda(Scene*, (void) {
				if (R_FAILED(location) && location != -1) {
					// something not working
					return getErrorScene(location, true);
				}
		
				bgLoopInit();
				if (location == -1) {
					return getHomeScene(); // load home
				}
				return getLocationScene(location);
			})), lambda(void, (void) {
				Result res;
				// first, we import the locally stored passes for reports to work
				reportInit();
				// next, we gotta wait for having internet
                DEBUG_PRINTF("Waiting internet\n");
				char url[50];
				snprintf(url, 50, "%s/ping", BASE_URL);
				int check_count = 0;
				int max_count = 100;
				while (true) {
					res = httpRequest("GET", url, 0, 0, 0, 0, 0);
					if (R_SUCCEEDED(res)) break;
					check_count++;
					if (check_count > max_count) {
						if (res == -CURLE_COULDNT_RESOLVE_HOST && max_count < 400) {
							max_count += 100;
							continue;
						}
						location = res;
						return;
					}
				}
				waitForCecdState(true, CEC_COMMAND_STOP, CEC_STATE_ABBREV_IDLE);
				initTitleData();
				doSlotExchange();
				res = getLocation();
				if (R_FAILED(res) && res != -1) {
					printf("ERROR failed to get location: %ld\n", res);
					location = -1;
				} else {
					location = res;
					if (location == -1) {
						printf("Got location home\n");
					} else {
						printf("Got location: %d\n", location);
					}
				}
			}));
		
			if (_PATCHES_VERSION_ > config.patches_version) {
				printf("New patches version to apply!\n");
				scene = getUpdatePatchesScene(scene);
			}
			
			if (_WELCOME_VERSION_ > config.welcome_version) {
				printf("New Welcome Screen to show!\n");
				scene = getWelcomeScene(scene);
			}
		}
	}

	scene->init(scene);

	while (aptMainLoop()) {
		Scene* new_scene = processScene(scene);
		if (!new_scene) break;
		if (new_scene != scene) {
			scene = new_scene;
			continue;
		}
		Scene* err_scene = get_new_error_scene();
		if (err_scene) {
			err_scene->pop_scene = scene;
			err_scene->init(err_scene);
			scene = err_scene;
		}
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
		C2D_SceneBegin(top);
		if (scene->is_popup) {
			scene->pop_scene->render(scene->pop_scene);
			C2D_Flush();
		}
		scene->render(scene);
		C3D_FrameEnd(0);
		svcSleepThread(1);
	}
	printf("\nExiting...\n");
	integrationExit();
	bgLoopExit();
	musicExit();
	C2D_Fini();
	C3D_Fini();
	curlExit();
	romfsExit();
	fsExit();
	frdExit();
	aptExit();
	nsExit();
	amExit();
	cfguExit();
	gfxExit();
	return 0;
}
