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

#include "music.h"
#include "api.h"
#include "config.h"
#include "utils.h"

#include <opus/opusfile.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define MUSIC_CHANNEL 8
#define OPUS_RATE (48000.f)
#define OPUS_CHANNELS ((size_t)2)
#define NUM_BUFFERS 4
#define OPUS_BUFFERSIZE ((size_t)(16 * 1024))
#define MAX_FILENAME_LEN 20
#define NUM_FILECACHES 2

typedef struct FileCacheEntry {
	unsigned char * data;
	size_t size;
	char name[MAX_FILENAME_LEN];
} FileCacheEntry;

static bool stop_playing = false;
static Thread music_thread = 0;
static char curfilename[MAX_FILENAME_LEN] = {0};
static FileCacheEntry file_cache[NUM_FILECACHES] = {0};
static size_t next_filecache = 0;

static void wait_for_state(bool playing) {
	int count = 0;
	while (ndspChnIsPlaying(MUSIC_CHANNEL) == !playing && count < 100000) {
		svcSleepThread(10);
		count++;
	}
}

__attribute__((optimize ("O3")))
static u64 fill_opus_buffer(OggOpusFile* opus_file, int16_t* buffer, int samples_to_read) {
	u64 samples_read = 0;

	while (samples_to_read > 0) {
		int samples_just_read = op_read_stereo(opus_file, buffer, samples_to_read);

		if (samples_just_read < 0) {
			return samples_just_read;
		} else if(samples_just_read == 0) {
			// EOF, loop file
			op_pcm_seek(opus_file, 0);
		}

		samples_read += samples_just_read;
		samples_to_read -= samples_just_read*2;
		buffer += samples_just_read*2;
	}
	return samples_read;
}

__attribute__((optimize ("O3")))
static void play_thread(void* p) {
	OggOpusFile* opus_file = p;
	// now allocate the buffers
	s16* buffers_mem = linearAlloc(OPUS_BUFFERSIZE * sizeof(s16) * NUM_BUFFERS);
	s16* buffers[NUM_BUFFERS];
	for (int i = 0; i < NUM_BUFFERS; i++) {
		buffers[i] = buffers_mem + i*OPUS_BUFFERSIZE;
	}
	ndspWaveBuf wavebuf[NUM_BUFFERS];
	memset(wavebuf, 0, sizeof(wavebuf));
	// now set the 3ds to be able to play the stuffs
	ndspChnReset(MUSIC_CHANNEL);
	ndspChnWaveBufClear(MUSIC_CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(MUSIC_CHANNEL, OPUS_RATE);
	ndspChnSetFormat(MUSIC_CHANNEL, NDSP_FORMAT_STEREO_PCM16);
	// set the wave buffers
	for (int i = 0; i < NUM_BUFFERS; i++) {
		int read = fill_opus_buffer(opus_file, buffers[i], OPUS_BUFFERSIZE);
		if (read <= 0) {
			logln(ERROR, "Music Fail: %d", read);
			goto fail;
		}
		wavebuf[i].nsamples = read;
		wavebuf[i].data_vaddr = buffers[i];
		ndspChnWaveBufAdd(MUSIC_CHANNEL, &wavebuf[i]);
		DSP_FlushDataCache(buffers[i], OPUS_BUFFERSIZE * sizeof(s16));
	}
	// now start the loop
	wait_for_state(true);
	while (!stop_playing) {
		svcSleepThread((u64)1000 * 10);
		// do nothing if the channel is paused
		if (ndspChnIsPaused(MUSIC_CHANNEL)) {
			continue;
		}
		
		for (int i = 0; i < NUM_BUFFERS; i++) {
			if (wavebuf[i].status == NDSP_WBUF_DONE) {
				int read = fill_opus_buffer(opus_file, buffers[i], OPUS_BUFFERSIZE);
				if (read < 0) {
					logln(ERROR, "Music Fail: %d", read);
					goto fail;
				}
				if (read == 0) {
					// song should loop
					op_pcm_seek(opus_file, 0);
					read = fill_opus_buffer(opus_file, buffers[i], OPUS_BUFFERSIZE);
				}
				wavebuf[i].nsamples = read;

				ndspChnWaveBufAdd(MUSIC_CHANNEL, &wavebuf[i]);
			}
			DSP_FlushDataCache(buffers[i], OPUS_BUFFERSIZE * sizeof(s16));
		}
	}
	
fail:
	linearFree(buffers_mem);
	ndspChnReset(MUSIC_CHANNEL);
	op_free(opus_file);
	threadExit(0);
}

static bool music_inited = false;

Result playMusic(const char* filename) {
	if (!music_inited) return ERROR_MUSIC_NOT_INITED;
	Result res = 0;
	// if we are already playing this file
	if (strcmp(filename, curfilename) == 0) return res;
	strncpy(curfilename, filename, sizeof(curfilename) - 1);
	curfilename[sizeof(curfilename) - 1] = '\0';
	stopMusic();
	if (!config.bg_music) {
		return res;
	}
	// first open the file
	char f[50];
	snprintf(f, 50, "romfs:/music/%s.opus", filename);
	int cache_slot = -1;
	for(int i = 0; i < NUM_FILECACHES; ++i)
	{
		if(file_cache[i].data && 0 == strncmp(filename, file_cache[i].name, MAX_FILENAME_LEN)) {
			cache_slot = i;
			logln(DEBUG, "found music cache slot %d", cache_slot);
			break;
		}
	}
	if(-1 == cache_slot) {
		logln(DEBUG, "adding music to cache slot %zu", next_filecache);
		FileCacheEntry * entry = &file_cache[next_filecache];
		if(entry->data) {
			logln(DEBUG, "clearing existing cache slot");
			free(entry->data);
			entry->data = NULL;
			entry->size = 0;
			entry->name[0] = '\0';
		}
		struct stat statbuf;
		if (0 != stat(f, &statbuf) || S_ISDIR(statbuf.st_mode)) {
			logln(ERROR, "failed to get file information for %s", f);
			return _e_errno();
		}
		FILE* file = fopen(f, "rb");
		if(!file) {
			logln(ERROR, "failed to open %s", f);
			return _e_errno();
		}
		entry->data = malloc(statbuf.st_size);
		if(!entry->data) {
			logln(ERROR, "failed to allocate buffer with size %jd", (intmax_t)statbuf.st_size);
			fclose(file);
			return _e_errno();
		}
		size_t len = fread(entry->data, 1, statbuf.st_size, file);
		if(len != statbuf.st_size) {
			logln(ERROR, "read length mismatch %zu != %jd", len, (intmax_t)statbuf.st_size);
			free(entry->data);
			entry->data = NULL;
			fclose(file);
			return _e_errno();
		}
		fclose(file);
		entry->size = len;
		strncpy(entry->name, filename, MAX_FILENAME_LEN - 1);
		entry->name[MAX_FILENAME_LEN - 1] = '\0';
		cache_slot = next_filecache;
		next_filecache = (next_filecache + 1) % NUM_FILECACHES;
	}
	logln(DEBUG, "playing music cache slot %d", cache_slot);
	OggOpusFile* opus_file = op_open_memory(file_cache[cache_slot].data, file_cache[cache_slot].size, (int*)&res);
	if (!opus_file) {
		return _e_errno();
	}
	stop_playing = false;
	music_thread = threadCreate(play_thread, opus_file, 26*1024, main_thread_prio()-10, -2, false);
	return res;
}

void getCurMusic(char filename[20]) {
	if (!music_inited) {
		filename[0] = '\0';
		return;
	}
	strncpy(filename, curfilename, sizeof(curfilename) - 1);
	filename[sizeof(curfilename) - 1] = '\0';
}

void stopMusic(void) {
	stop_playing = true;
	wait_for_state(false);
	if (music_thread) {
		_e(threadJoin(music_thread, U64_MAX));
		threadFree(music_thread);
		music_thread = 0;
	}
}

void toggleBgMusic(void) {
	if (!music_inited) return;
	config.bg_music = !config.bg_music;
	if (!config.bg_music) {
		stopMusic();
	} else {
		char filename[sizeof(curfilename)];
		strncpy(filename, curfilename, sizeof(filename) - 1);
		filename[sizeof(filename) - 1] = '\0';
		memset(curfilename, 0, sizeof(curfilename));
		playMusic(filename);
	}
	configWrite();
}

static u8* dsp_buf = 0;

void musicInit(void) {
	if (music_inited) return;
	Result res = ndspInit();
	if (!R_FAILED(res)) {
		music_inited = true;
		return;
	}
	
	// stuffs failed, mew
	// let's try if this works if we manually extract the ndsp firm
	
	static const u64 tidhigh = 0x0004003000000000;
	static const u32 tidlow_home[6] = {
		0x0000f202, // usa
		0x00008202, // jpn
		0x00009802, // eur
		0x0000A102, // chn
		0x0000A902, // kor
		0x0000B102, // twn
	};
	Handle file;
	for (int i = 0; i < 6; i++) {
		u64 tid = tidhigh | tidlow_home[i];
		u32 archPathRaw[] = {tid & 0xFFFFFFFF, (tid >> 32) & 0xFFFFFFFF, 0, 0x00000000};
		FS_Path archPath = {PATH_BINARY, 0x10, (u8*)archPathRaw};
		static const u32 filePathRaw[] = {0x00000000, 0x00000000, 0x00000002, 0x646F632E, 0x00000065};
		FS_Path filePath = {PATH_BINARY, 0x14, (u8*)filePathRaw};
		res = FSUSER_OpenFileDirectly(&file, (FS_ArchiveID)0x2345678a, archPath, filePath, FS_OPEN_READ, 0);
		if (R_FAILED(res)) continue;
		break;
	}
	u64 fileSize = 0;
	u32 compressed_size = 0;
	res = FSFILE_GetSize(file, &fileSize);
	if (R_FAILED(res)) {
		FSFILE_Close(file);
		return;
	}
	u8* compressed = malloc(fileSize);
	if (!compressed) {
		FSFILE_Close(file);
		return;
	}
	res = FSFILE_Read(file, &compressed_size, 0x0, compressed, fileSize);
	FSFILE_Close(file);
	if (R_FAILED(res) || compressed_size != fileSize) {
		free(compressed);
		return;
	}
	u32 decompressed_size = blz_decompress_size(compressed, compressed_size);
	u8* decompressed = malloc(decompressed_size);
	if (!decompressed) {
		free(compressed);
		return;
	}
	bool success = blz_decompress(compressed, compressed_size, decompressed, decompressed_size);
	free(compressed);
	if (!success) {
		free(decompressed);
		return;
	}
	
	const char* magic = "DSP1";
	u8* dsp_loc = memsearch(decompressed, decompressed_size, (u8*)magic, 4);
	if (!dsp_loc) {
		free(decompressed);
		return;
	}
	u32 dsp_size = *(u32*)(dsp_loc + 4);
	dsp_loc -= 0x100;
	
	dsp_buf = malloc(dsp_size);
	if (!dsp_buf) {
		free(decompressed);
		return;
	}
	memcpy(dsp_buf, dsp_loc, dsp_size);
	free(decompressed);
	
	// ok, we finally got the firmware in dsp_buf
	ndspUseComponent(dsp_buf, dsp_size, 0xFF, 0xFF);
	res = _e(ndspInit());
	if (!R_FAILED(res)) {
		music_inited = true;
	}
}

void musicExit(void) {
	stopMusic();
	ndspExit();
	if (dsp_buf) {
		free(dsp_buf);
		dsp_buf = 0;
	}
	for(int i = 0; i < NUM_FILECACHES; ++i) {
		if(file_cache[i].data) {
			free(file_cache[i].data);
			file_cache[i].data = NULL;
		}
	}
	music_inited = false;
}
