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

// A lot of code around qr scanning from FBI and Anemone, thanks a lot!

#include "scan_qr.h"
#include "../qr.h"
#include "../render.h"
#include "../music.h"
#include "loading.h"
#include "prompt.h"
#include <stdlib.h>
#include <malloc.h>
#define N(x) scenes_scan_qr_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_B_GO_BACK_LEN + STR_X_SWITCH_CAMERA_LEN)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_b_back;
	C2D_Text g_x_switch_camera;
	
	struct quirc* qr_context;
	u16* buffer;
	bool is_inner_cam;
	Handle mutex;
	volatile bool finished;
	volatile bool buffer_updated_ui;
	volatile bool buffer_updated_processing;
	C3D_Tex tex;
	Handle cancel_event;
	Thread cam_thread;
	char prev_music[20];
} N(DataStruct);

QrBuffer* N(qr_buffer) = 0;
void* N(qr_payload) = 0;

void N(captureCamThread)(void* arg) {
	N(DataStruct)* data = (N(DataStruct)*)arg;
	u32 transfer_unit;
	size_t buffer_size = SCREEN_TOP_WIDTH * SCREEN_TOP_HEIGHT * sizeof(u16);
	u16* buffer = linearAlloc(buffer_size);
	if (!buffer) return;
	
	Handle cam_events[3] = {0};
	cam_events[0] = data->cancel_event;
	
	u32 cam = data->is_inner_cam ? SELECT_IN1 : SELECT_OUT1;
	
	camInit();
	CAMU_SetSize(cam, SIZE_CTR_TOP_LCD, CONTEXT_A);
	CAMU_SetOutputFormat(cam, OUTPUT_RGB_565, CONTEXT_A);
	CAMU_SetFrameRate(cam, FRAME_RATE_30);
	CAMU_SetNoiseFilter(cam, true);
	CAMU_SetAutoExposure(cam, true);
	CAMU_SetAutoWhiteBalance(cam, true);
	CAMU_Activate(cam);
	CAMU_GetBufferErrorInterruptEvent(&cam_events[2], PORT_CAM1);
	CAMU_SetTrimming(PORT_CAM1, false);
	CAMU_GetMaxBytes(&transfer_unit, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT);
	CAMU_SetTransferBytes(PORT_CAM1, transfer_unit, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT);
	CAMU_ClearBuffer(PORT_CAM1);
	CAMU_SetReceiving(&cam_events[1], buffer, PORT_CAM1, buffer_size, transfer_unit);
	CAMU_StartCapture(PORT_CAM1);
	
	bool stop = false;
	while (!stop) {
		s32 index = 0;
		svcWaitSynchronizationN(&index, cam_events, 3, false, U64_MAX);
		switch(index) {
			case 0:
				// cancel event
				stop = true;
				break;
			case 1:
				svcCloseHandle(cam_events[1]);
				cam_events[1] = 0;

				svcWaitSynchronization(data->mutex, U64_MAX);
				memcpy(data->buffer, buffer, buffer_size);
				data->buffer_updated_ui = true;
				data->buffer_updated_processing = true;
				svcReleaseMutex(data->mutex);

				CAMU_SetReceiving(&cam_events[1], buffer, PORT_CAM1, buffer_size, transfer_unit);
				break;
			case 2:
				svcCloseHandle(cam_events[1]);
				cam_events[1] = 0;

				CAMU_ClearBuffer(PORT_CAM1);
				CAMU_SetReceiving(&cam_events[1], buffer, PORT_CAM1, buffer_size, transfer_unit);
				CAMU_StartCapture(PORT_CAM1);
				break;
		}
	}
	CAMU_StopCapture(PORT_CAM1);
	bool busy = false;
	while(R_SUCCEEDED(CAMU_IsBusy(&busy, PORT_CAM1)) && busy) {
		svcSleepThread(1000000);
	}
	CAMU_ClearBuffer(PORT_CAM1);
	CAMU_Activate(SELECT_NONE);
	camExit();
	linearFree(buffer);
	for (int i = 1; i < 3; i++) {
		if (cam_events[i] != 0) {
			svcCloseHandle(cam_events[i]);
		}
	}
}

void N(stopCamera)(Scene* sc) {
	svcSignalEvent(_data->cancel_event);
	threadJoin(_data->cam_thread, U64_MAX);
	threadFree(_data->cam_thread);
	svcClearEvent(_data->cancel_event);
	_data->cam_thread = 0;
}

bool N(startCamera)(Scene* sc) {
	if (_data->cam_thread) N(stopCamera)(sc);
	_data->cam_thread = threadCreate(N(captureCamThread), _data, 0x10000, main_thread_prio() - 1, -2, false);
	return _data->cam_thread != 0;
}

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	memset(_data, 0, sizeof(N(DataStruct)));
	if (!N(qr_buffer)) {
		N(qr_buffer) = malloc(sizeof(QrBuffer));
		if (!N(qr_buffer)) {
			free(_data);
			sc->d = NULL;
			return;
		}
	}
	
	getCurMusic(_data->prev_music);
	playMusic("qr_code");
	
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	TextLangParse(&_data->g_b_back, _data->g_staticBuf, str_b_go_back);
	TextLangParse(&_data->g_x_switch_camera, _data->g_staticBuf, str_x_switch_camera);
	_data->buffer = malloc(SCREEN_TOP_WIDTH * SCREEN_TOP_HEIGHT * sizeof(u16));
	if (!_data->buffer) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		sc->d = NULL;
		return;
	}
	_data->qr_context = quirc_new();
	if (!_data->qr_context) {
		free(_data->buffer),
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		sc->d = NULL;
		return;
	}
	if (quirc_resize(_data->qr_context, SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT) != 0) {
		quirc_destroy(_data->qr_context);
		free(_data->buffer);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		sc->d = NULL;
		return;
	}
	C3D_TexInit(&_data->tex, 512, 256, GPU_RGB565);
	svcCreateMutex(&_data->mutex, false);
	svcCreateEvent(&_data->cancel_event, RESET_STICKY);
	_data->is_inner_cam = false;
	if (!N(startCamera)(sc)) {
		C3D_TexDelete(&_data->tex);
		svcCloseHandle(_data->cancel_event);
		svcCloseHandle(_data->mutex);
		quirc_destroy(_data->qr_context);
		free(_data->buffer);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
		sc->d = NULL;
		return;
	}
}

void N(render)(Scene* sc) {
	if (!_data) return;
	svcWaitSynchronization(_data->mutex, U64_MAX);
	if (_data->buffer_updated_ui) {
		for (u32 y = 0; y < SCREEN_TOP_HEIGHT; y++) {
			const u32 src_pos = y * SCREEN_TOP_WIDTH;
			for (u32 x = 0; x < SCREEN_TOP_WIDTH; x++) {
				const u32 dst_pos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
				((u16 *)_data->tex.data)[dst_pos] = _data->buffer[src_pos + x];
			}
		}
		_data->buffer_updated_ui = false;
	}
	svcReleaseMutex(_data->mutex);
	static const Tex3DS_SubTexture subt3x = { SCREEN_TOP_WIDTH, SCREEN_TOP_HEIGHT, 0.0, 1.0, SCREEN_TOP_WIDTH*1.0/512.0, 1.0 - (SCREEN_TOP_HEIGHT*1.0/256.0) };
	C2D_DrawImageAt((C2D_Image){ &_data->tex, &subt3x }, 0, 0, 0, NULL, 1, 1);
	
	C2D_DrawRectSolid(0, SCREEN_TOP_HEIGHT - 17, 0, SCREEN_TOP_WIDTH, 17, clr_netpass_green);
	C2D_DrawText(&_data->g_b_back, C2D_AlignLeft | C2D_WithColor, 2, SCREEN_TOP_HEIGHT - 16, 0, 0.5, 0.5, clr_white);
	C2D_DrawText(&_data->g_x_switch_camera, C2D_AlignCenter | C2D_WithColor, SCREEN_TOP_WIDTH / 2, SCREEN_TOP_HEIGHT - 16, 0, 0.5, 0.5, clr_white);
}

void N(exit)(Scene* sc) {
	if (_data) {
		N(stopCamera)(sc);
		C3D_TexDelete(&_data->tex);
		
		svcCloseHandle(_data->cancel_event);
		svcCloseHandle(_data->mutex);
		quirc_destroy(_data->qr_context);
		free(_data->buffer);
		playMusic(_data->prev_music);
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
	if (N(qr_payload)) {
		free(N(qr_payload));
		N(qr_payload) = NULL;
	}
}

SceneResult N(process)(Scene* sc) {
	if (!_data) return scene_pop;
	svcWaitSynchronization(_data->mutex, U64_MAX);
	if (_data->buffer_updated_processing) {
		int w = 0;
		int h = 0;
		u8* qr_buf = quirc_begin(_data->qr_context, &w, &h);
		for (int y = 0; y < h; y++) {
			const int actual_y = y * w;
			for (int x = 0; x < w; x++) {
				const int actual_off = actual_y + x;
				const u16 px = _data->buffer[actual_off];
				qr_buf[actual_off] = (u8)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
			}
		}
		_data->buffer_updated_processing = false;
		svcReleaseMutex(_data->mutex);
		quirc_end(_data->qr_context);
		int qr_count = quirc_count(_data->qr_context);
		for (int i = 0;i < qr_count; i++) {
			struct quirc_code code;
			quirc_extract(_data->qr_context, i, &code);
			struct quirc_data qr_data;
			if (quirc_decode(&code, &qr_data) == 0) {
				qr_buffer_from_quirc_data(N(qr_buffer), &qr_data);
				if (!qr_buf_equal(N(qr_buffer), (u8*)"NPQR", 4)) {
					logln(ERROR, "Invalid netpass qr code!");
					continue;
				}
				u32 method = qr_read_u32(N(qr_buffer));
				logln(INFO, "Method: %ld", method);
				switch (method) {
					case QR_METHOD_VERIFY: {
						sc->next_scene = getPromptScene(str_prompt_verify, getLoadingScene(NULL, lambda(void, (void) {
							Result res = qr_verify(N(qr_buffer));
							if (R_FAILED(res)) {
								_e(res);
								logln(ERROR, "Verification failed: %lx", res);
							} else {
								logln(INFO, "Verification successful!");
							}
						})));
						return scene_push;
					};
					case QR_METHOD_JOIN_EVENT_ROOM: {
						N(qr_payload) = malloc(sizeof(QrJoinEventRoomPayload));
						if (!N(qr_payload)) {
							_e(ERROR_OUT_OF_MEMORY);
							break;
						}
						qr_parse_join_event_room(N(qr_buffer), N(qr_payload));
						char* message = malloc(1000);
						if (!message) {
							free(N(qr_payload));
							N(qr_payload) = 0;
							_e(ERROR_OUT_OF_MEMORY);
							break;
						}
						snprintf(message, 1000, _s(str_enter_event_location), ((QrJoinEventRoomPayload*)N(qr_payload))->name);
						C2D_Font font = _font(str_enter_event_location);
						sc->next_scene = getPromptSceneStr(message, font, getLoadingScene(NULL, lambda(void, (void) {
							Result res = setEventLocation(((QrJoinEventRoomPayload*)N(qr_payload))->uuid);
							if (R_FAILED(res)) {
								_e(res);
								logln(ERROR, "Failed to join event location: %lx", res);
							} else {
								getLocation();
								triggerDownloadInboxes();
							        logln(INFO, "Successfully joined event location!");
							}
						})));
						return scene_push;
					};
					case QR_METHOD_DL_PASS: {
						sc->next_scene = getPromptScene(str_prompt_dl_pass, getLoadingScene(NULL, lambda(void, (void) {
							Result res = qr_dl_pass(N(qr_buffer));
							if (R_FAILED(res)) {
								_e(res);
							        logln(INFO, "Pass DL failed: %lx", res);
							} else {
								logln(INFO, "Pass DL successful!");
							}
						})));
						return scene_push;
					}
					default:
						logln(ERROR, "Unknown method %ld", method);
				}
			}
		}
	} else {
		svcReleaseMutex(_data->mutex);
	}

	hidScanInput();
	u32 kDown = hidKeysDown();
	
	if (kDown & KEY_X) {
		N(stopCamera)(sc);
		_data->is_inner_cam = !_data->is_inner_cam;
		N(startCamera)(sc);
	}
	
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getScanQrScene(void) {
	Scene* scene = createScene(0);
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render_top = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	return scene;
}
