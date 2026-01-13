/**
 * NetPass
 * Copyright (C) 2024-2025 Sorunome
 *               2025 yabobay
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
#include <unistd.h> // TODO: check if needed
#include "log.h"
#include "scene.h"
#include "api.h"
#include "cecd.h"
#include "curl-handler.h"
#include "config.h"
#include "music.h"
#include "integration.h"
#include "scenes/download_progress.h"
#include "scenes/switch.h"
#include "render.h"

CurlReply* ping_reply = 0;
Result ping_res = 0;
PingResponse ping_response = {0};
char* filename_3dsx = 0;

Scene* load_is_banned(void) {
	char ban_start[40];
	char ban_end[40];
	struct tm tm = {0};
	if (ping_response.ban.time_start.year) {
		cecTimeToTm(&ping_response.ban.time_start, &tm);
		n_strftime(ban_start, sizeof(ban_start), _s(str_date), &tm);
	} else {
		strncpy(ban_start, "N/A", sizeof(ban_start));
	}
	if (ping_response.ban.time_end.year) {
		cecTimeToTm(&ping_response.ban.time_end, &tm);
		n_strftime(ban_end, sizeof(ban_end), _s(str_date), &tm);
	} else {
		strncpy(ban_end, "N/A", sizeof(ban_end));
	}
	
	Scene* scene = getSettingsScene();
	char* message = malloc(1000);
	if (message) {
		snprintf(message, 1000, _s(str_banned), ping_response.ban.reason, ban_start, ban_end);
		C2D_Font font = _font(str_banned);
		Scene* ban_scene = getInfoSceneStr(message, font);
		ban_scene->pop_scene = scene;
		scene = ban_scene;
	}
	return scene;
}

CurlReply* reply_new_version = 0;
Scene* load_new_version(Scene* scene) {
	char* message = malloc(1000);
	if (message) {
		snprintf(message, 1000, _s(str_new_version), ping_response.version.major, ping_response.version.minor, ping_response.version.patch);
		C2D_Font font = _font(str_new_version);
		static const char* filename_cia = "sdmc:/config/netpass/netpass.cia";
		static const char* filename_3dsx_tmp = "sdmc:/config/netpass/netpass.3dsx";
		Scene* version_scene = getPromptSceneStr(message, font, getDownloadProgressScene(&reply_new_version, getSwitchScene(lambda(Scene*, (void) {
			curlFreeHandler(reply_new_version->offset);
			if (R_FAILED(ping_res)) {
				return getInfoScene(str_new_version_failed);
			}
			// things were successful, let's restart!
			return getLoadingScene(getStopScene(), lambda(void, (void) {
				aptSetHomeAllowed(true);
				if (filename_3dsx) return;
				AM_TitleEntry info;
				get_cia_info(filename_cia, &info);
				Result res = 0;
				res = _e(APT_PrepareToDoApplicationJump(0, info.titleID, get_title_destination(info.titleID)));
				if (R_FAILED(res)) goto fail;
				u8 param[0x300];
				u8 hmac[0x20];
				res = _e(APT_DoApplicationJump(param, sizeof(param), hmac));
				if (R_FAILED(res)) goto fail;
			fail:
				while(true) {
					svcSleepThread(100000000);
				}
			}));
		})), lambda(void, (void) {
			if (filename_3dsx) {
				// this is easy, just download and overwrite the file
				// we first download it to a different file to prevent weird glitches with music and whatnot
				mkdir_p(filename_3dsx_tmp);
				logln(DEBUG, "filename: %s\n", filename_3dsx);
				logln(DEBUG, "tmp filename: %s\n", filename_3dsx_tmp);
				ping_res = _e(httpRequest("GET", BASE_URL "/netpass.3dsx", 0, 0, &reply_new_version, filename_3dsx_tmp));
				if (R_FAILED(ping_res)) return;
				aptSetHomeAllowed(false);
				unlink(filename_3dsx);
				if (cp(filename_3dsx_tmp, filename_3dsx) != 0) {
					ping_res = _e_errno();
					return;
				}
				return;
			}
			// ok, we have a cia file. this will be a tad harder.
			mkdir_p(filename_cia);
			ping_res = _e(httpRequest("GET", BASE_URL "/netpass.cia", 0, 0, &reply_new_version, filename_cia));
			if (R_FAILED(ping_res)) return;
			aptSetHomeAllowed(false);
			ping_res = install_cia(filename_cia);
		})));
		version_scene->pop_scene = scene;
		scene = version_scene;
	}
	return scene;
}

Scene* initial_scene(void) {
	if (R_FAILED(ping_res)) {
		// something not working
		_e(ping_res);
		return getSettingsScene();
	}
	if (ping_response.ban.is_banned) {
		// we are banned
		return load_is_banned();
	}
	bgLoopInit();
	Scene* scene;
	if (location.id == -1) {
		scene = getHomeScene(); // load home
	} else {
		scene = getLocationScene(location.id);
	}

	if (ping_response.version.new_version_available) {
		scene = load_new_version(scene);
	} else if (ping_response.message.message) {
		Scene* message_scene = getInfoSceneStr(ping_response.message.message, 0);
		message_scene->pop_scene = scene;
		scene = message_scene;
	}
	return scene;
}

void initial_load(void) {
	// first, we import the locally stored passes for reports to work
	reportInit();
	// next, we gotta wait for having internet
	logln(DEBUG, "Waiting internet\n");
	char url[50];
	snprintf(url, 50, "%s/ping2", BASE_URL);
	int check_count = 0;
	int max_count = 100;
	while (true) {
		ping_res = httpRequest("GET", url, 0, 0, &ping_reply, 0);
		if (R_SUCCEEDED(ping_res)) break;
		check_count++;
		curlFreeHandler(ping_reply->offset);
		if (ERROR_IS_HTTP(ping_res)) return;
		if (ERROR_IS_CURL(ping_res) && ping_res == -CURLE_PEER_FAILED_VERIFICATION) return;
		if (check_count > max_count) {
			if (ping_res == -CURLE_COULDNT_RESOLVE_HOST && max_count < 400) {
				max_count += 100;
				continue;
			}
			return;
		}
	}
	readPingResponse(&ping_response, ping_reply->ptr, ping_reply->len);
	curlFreeHandler(ping_reply->offset);
	if (ping_response.ban.is_banned) return;
	_e(waitForCecdState(true, CEC_COMMAND_STOP, CEC_STATE_ABBREV_IDLE));
	initTitleData();
	doSlotExchangeRetry(true);
	Result ping_res = getLocation();
	if (R_FAILED(ping_res)) {
		logln(ERROR, "failed to get location: %ld", ping_res);
	} else {
		char uuidstr[37];
		format_uuid(uuidstr, location.uuid);
		logln(INFO, "Got location: %ld %s", location.id, uuidstr);
	}
}

int main(int nargs, char** argv) {
	osSetSpeedupEnable(true); // enable speedup on N3DS

	gfxInitDefault();
	miscInit();
	_e(cfguInit());
	_e(amInit());
	_e(nsInit());
	_e(aptInit());
	_e(frdInit(false));
	_e(fsInit());
	_e(cecdInit());
	
	if (nargs >= 1) {
		filename_3dsx = argv[0];
	}

	configInit(); // must be after cecdInit()
	logInit(); // must be after configInit();
	
	bool output_bottom_screen = config.log_output != LogOutputBottomScreen;
	
	if (!output_bottom_screen) {
		consoleInit(GFX_BOTTOM, NULL);
	}

	LogMessage* log = log_start(INFO);
	log_multi(log, "Starting NetPass v%d.%d.%d", _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
#ifdef _VERSION_GIT_SHA_
	log_multi(log, "+%s", _VERSION_GIT_SHA_);
#endif
	log_end(log);
	// sadly the pretty outline text uses a *lot* of cmdbuf size,
	// so we need to increase our cmdbuf size
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 2);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
	C2D_Prepare();
	_e(romfsInit());
	init_main_thread_prio();

	logln(DEBUG, "DEBUG ON");

	_e(curlInit());
	srand(time(NULL));

	stringsInit(); // must be after configInit()
	musicInit(); // must be after romfsInit()
	renderInit(); // must be after romfsInit()

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
		_e(archiveMount(ARCHIVE_SHARED_EXTDATA, extdata_path, "sharedextdata_b"));
		_e(FSUSER_OpenArchive(&sharedextdata_b, ARCHIVE_SHARED_EXTDATA, extdata_path));
	}
	
	_e(playMusic("home")); // start the default music

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget* bottom = output_bottom_screen ? C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT) : NULL;

	Scene* scene;
	{
		OS_VersionBin ver;
		Result res = _e(get_os_version(&ver));
		if (R_FAILED(res)) {
			logln(INFO, "osGetSystemVersionData res: %08lX", res);
			
			logln(INFO, "Detected system version (cver): %d.%d.%d%c", ver.mainver, ver.minor, ver.build, ver.region);
			u8 region;
			res = CFGU_SecureInfoGetRegion(&region);
			logln(INFO, "Get region (%08lX): %d", res, region);
		}

	
		if (SYSTEM_VERSION(ver.mainver, ver.minor, ver.build) < SYSTEM_VERSION(11, 15, 0)) {
			scene = getBadOsVersionScene();
		} else {
			// somehow cpp check fails with this lambda for the ptr->int return type check
			// as it does not even compile if we were to cast the returns to ints, this is clearly a cppcheck bug
			// cppcheck-suppress CastAddressToIntegerAtReturn
			scene = getLoadingScene(getSwitchScene(initial_scene), initial_load);
		
			if (_PATCHES_VERSION_ > config.patches_version) {
				logln(INFO, "New patches version to apply!");
				scene = getUpdatePatchesScene(scene);
			}
			
			if (_WELCOME_VERSION_ > config.welcome_version) {
				logln(INFO, "New Welcome Screen to show!");
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
		if (top) {
			C2D_TargetClear(top, 0xFFFFFFFF);
			C2D_SceneBegin(top);
			if (scene->is_popup && scene->pop_scene) {
				renderTopScene(scene->pop_scene);
			}
			renderTopScene(scene);
		}
		
		if (bottom) {
			C2D_TargetClear(bottom, 0xFFFFFFFF);
			C2D_SceneBegin(bottom);
			if (scene->is_popup && scene->pop_scene) {
				renderBottomScene(scene->pop_scene);
			}
			renderBottomScene(scene);
			renderBottomScreen();
		}
		C3D_FrameEnd(0);
		svcSleepThread(1);
	}
	logln(INFO, "Exiting...");
	renderExit();
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
	logExit();
	return 0;
}
