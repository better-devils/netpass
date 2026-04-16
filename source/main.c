/**
 * NetPass
 * Copyright (C) 2024-2026 Sorunome
 *               2025 yabobay
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

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
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
#include "scenes/info.h"
#include "scenes/loading.h"
#include "scenes/settings.h"
#include "scenes/switch.h"
#include "render.h"
#include "utils.h"

static Result ping_res = 0;
static PingResponse ping_response = {0};
char* filename_3dsx = 0;
static time_t server_date = -1;

static Scene* load_not_authenticated(void) {
	Scene* scene = getSettingsScene();
	Scene* info_scene = getInfoScene(str_failed_to_authenticate);
	info_scene->pop_scene = scene;
	return info_scene;
}

static Scene* load_is_banned(void) {
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

static CurlReply* reply_new_version = 0;
static Scene* load_new_version(Scene* scene) {
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
			return getLoadingScene(getRestartScene(), lambda(void, (void) {
				aptSetHomeAllowed(true);
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

static Scene* initial_scene(void) {
	if (ping_res == -423) {
		// console had already been registered
		Scene* scene = getSettingsScene();
		Scene* info_scene = getInfoScene(str_already_registered);
		info_scene->pop_scene = scene;
		return info_scene;
	}
	if (R_FAILED(ping_res) || !ping_response.is_authenticated) {
		// something not working
		return getLoadingScene(getSwitchScene(lambda(Scene*, (void) {
			if (server_date > -1) {
				logln(INFO, "time to set the time!");
				return getSetTimeScene(server_date);
			}
			if (R_FAILED(ping_res)) {
				_e(ping_res);
				return getSettingsScene();
			}
			return load_not_authenticated();
		})), lambda(void, (void) {
			CurlReply* reply;
			Result res = httpRequest("GET", "http://netpass.cafe/conntest", 0, 0, &reply, 0);
			if (R_FAILED(res)) {
				curlFreeHandler(reply->offset);
				if (R_SUCCEEDED(ping_res)) ping_res = res;
				return;
			}
			time_t date = reply->date;
			curlFreeHandler(reply->offset);
			if (date == -1) return;
			time_t now = time(NULL);
			
			if (llabs(now - date) > 60*60*18) {
				server_date = date;
			}
		}));
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

static Result check_internet_call(char* url, CurlReply** reply) {
	int check_count = 0;
	int max_count = 100;
	while (true) {
		Result res = httpRequest("GET", url, 0, 0, reply, 0);
		if (R_SUCCEEDED(res)) break;
		check_count++;
		curlFreeHandler((*reply)->offset);
		if (ERROR_IS_HTTP(res)) return res;
		if (ERROR_IS_CURL(res) && res == -CURLE_PEER_FAILED_VERIFICATION) return res;
		if (check_count > max_count) {
			if (res == -CURLE_COULDNT_RESOLVE_HOST && max_count < 400) {
				max_count += 100;
				continue;
			}
			return res;
		}
	}
	return 0;
}

static void initial_load(void) {
	// first, we init the report stuffs
	reportInit();
	ping_res = 0;
	char url[50];
	CurlReply* reply;
	logln(DEBUG, "Waiting internet\n");
	bool have_nid_pwd = access(PATH_NID_PWD, F_OK) == 0;
	bool have_nid_pwd_bak = access(PATH_NID_PWD_BAK, F_OK) == 0;
	bool have_mac = access(PATH_MAC, F_OK) == 0;
	bool have_mac_bak = access(PATH_MAC_BAK, F_OK) == 0;
	if (have_mac_bak && !have_mac) {
		u8 mac[6];
		u8 mac_cmp[6];
		ping_res = _e(getMac(mac));
		if (R_FAILED(ping_res)) return;
		FILE* f = fopen(PATH_MAC_BAK, "wb");
		if (!f) {
			ping_res = _e_errno();
			return;
		}
		if (fread(mac_cmp, 6, 1, f) != 1) {
			ping_res = _e_errno();
			fclose(f);
			return;
		}
		fclose(f);
		if (memcmp(mac, mac_cmp, 6) != 0) {
			logln(INFO, "Bad mac backup, pretending the backup files don't exist");
			remove(PATH_MAC_BAK);
			remove(PATH_NID_PWD_BAK);
			have_nid_pwd_bak = false;
			have_mac_bak = false;
		}
	}
	if (have_mac && !have_mac_bak) {
		ping_res = _e(cp(PATH_MAC, PATH_MAC_BAK));
		if (R_FAILED(ping_res)) return;
		have_mac_bak = true;
	}
	
	if (!have_nid_pwd && !have_nid_pwd_bak) {
		// we gotta register
		logln(INFO, "First time opening NetPass, registering console...");
		snprintf(url, 50, "%s/register", BASE_URL);
		ping_res = check_internet_call(url, &reply);
		if (R_FAILED(ping_res)) {
			logln(INFO, "Failed to register: %08lx", ping_res);
			return;
		}
		mkdir_p(PATH_NID_PWD);
		FILE* f = fopen(PATH_NID_PWD, "wb");
		if (!f) {
			ping_res = _e_errno();
			logln(INFO, "Failed to open password file");
			curlFreeHandler(reply->offset);
			return;
		}
		if (fwrite(reply->ptr, reply->len, 1, f) != 1) {
			ping_res = _e_errno();
			fclose(f);
			logln(INFO, "Failed to write to password file");
			curlFreeHandler(reply->offset);
			return;
		}
		fclose(f);
		curlFreeHandler(reply->offset);
		have_nid_pwd = true;
		have_nid_pwd_bak = false;
		logln(INFO, "Console registered successfully!");
	} else if (!have_nid_pwd && have_nid_pwd_bak) {
		ping_res = _e(cp(PATH_NID_PWD_BAK, PATH_NID_PWD));
		if (R_FAILED(ping_res)) return;
		have_nid_pwd = true;
	}
	// potentially copy from real to backup
	if (have_nid_pwd && !have_nid_pwd_bak) {
		ping_res = _e(cp(PATH_NID_PWD, PATH_NID_PWD_BAK));
		if (R_FAILED(ping_res)) return;
	}
	// next, we gotta wait for having internet
	snprintf(url, 50, "%s/ping2", BASE_URL);
	ping_res = _e(check_internet_call(url, &reply));
	if (R_FAILED(ping_res)) return;
	readPingResponse(&ping_response, reply->ptr, reply->len);
	curlFreeHandler(reply->offset);
	if (ping_response.ban.is_banned || !ping_response.is_authenticated) return;
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
	_e(acInit());
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

	
	// mount nand so that we can use it for some things
	{
		_e(archiveMount(ARCHIVE_NAND_RW, fsMakePath(PATH_EMPTY, ""), "nand"));
	}
	
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
	reportExit();
	renderExit();
	integrationExit();
	bgLoopExit();
	musicExit();
	C2D_Fini();
	C3D_Fini();
	curlExit();
	romfsExit();
	logExit();
	fsExit();
	frdExit();
	aptExit();
	nsExit();
	amExit();
	acExit();
	cfguExit();
	gfxExit();
	return 0;
}
