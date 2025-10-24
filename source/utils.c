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

#include "utils.h"
#include "strings.h"
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#define _NJ_INCLUDE_HEADER_ONLY
#include "nanojpeg.c"
#include "lodepng/lodepng.h"

// cppcheck-suppress unusedFunction
void* cecGetExtHeader(CecMessageHeader* msg, u32 type) {
	u32 counter = sizeof(CecMessageHeader);
	while (counter < msg->total_header_size) {
		u32 this_type = ((u32*)(((u8*)msg) + counter))[0];
		u32 this_size = ((u32*)(((u8*)msg) + counter))[1];
		if (this_type == type) return ((u8*)msg) + counter;
		counter += this_size;
		// we might have to do extra aligning
		if (counter %4) counter += 4 - counter % 4;
	}
	return NULL;
}

// cppcheck-suppress unusedFunction
u32 cecGetExtHeaderSize(CecMessageHeader* msg, u32 type) {
	u32 counter = sizeof(CecMessageHeader);
	while (counter < msg->total_header_size) {
		u32 this_type = ((u32*)(((u8*)msg) + counter))[0];
		u32 this_size = ((u32*)(((u8*)msg) + counter))[1];
		if (this_type == type) return this_size;
		counter += this_size;
		// we might have to do extra aligning
		if (counter %4) counter += 4 - counter % 4;
	}
	return 0;
}

size_t n_strftime(char* str, size_t count, const char* format, const struct tm* tp) {
	char* tmpstr = malloc(count);
	if (!tmpstr) return 0;
	strncpy(tmpstr, format, count);
	char* pos;
	
	u8 lang = 0;
	u8 sys_lang = get_language();
	// no need to handle language not found, as on program startup that is already ensured
	// so during normal runtime we don't have to check this
	for (lang = 0; lang < NUM_LANGUAGES; lang++) {
		if (all_languages[lang] == sys_lang) break;
	}
	// if not found, default to zero
	if (lang >= NUM_LANGUAGES) lang = 0;
	
	// months abbreviation
	if (lc_time_all.months_abbr[lang][0] && 0 != (pos = strstr(tmpstr, "%b"))) {
		*pos = 0;
		pos += 2;
		snprintf(str, count, "%s%s%s", tmpstr, lc_time_all.months_abbr[lang][tp->tm_mon], pos);
		strncpy(tmpstr, str, count);
	}
	
	// months
	if (lc_time_all.months[lang][0] && 0 != (pos = strstr(tmpstr, "%B"))) {
		*pos = 0;
		pos += 2;
		snprintf(str, count, "%s%s%s", tmpstr, lc_time_all.months[lang][tp->tm_mon], pos);
		strncpy(tmpstr, str, count);
	}
	
	// weekdays abbreviation
	if (lc_time_all.weekdays_abbr[lang][0] && 0 != (pos = strstr(tmpstr, "%a"))) {
		*pos = 0;
		pos += 2;
		snprintf(str, count, "%s%s%s", tmpstr, lc_time_all.weekdays_abbr[lang][tp->tm_wday], pos);
		strncpy(tmpstr, str, count);
	}
	
	// weekdays
	if (lc_time_all.weekdays[lang][0] && 0 != (pos = strstr(tmpstr, "%A"))) {
		*pos = 0;
		pos += 2;
		snprintf(str, count, "%s%s%s", tmpstr, lc_time_all.weekdays[lang][tp->tm_wday], pos);
		strncpy(tmpstr, str, count);
	}
	
	// am/pm
	if (lc_time_all.ampm[lang][0] && 0 != (pos = strstr(tmpstr, "%p"))) {
		*pos = 0;
		pos += 2;
		snprintf(str, count, "%s%s%s", tmpstr, lc_time_all.ampm[lang][tp->tm_hour >= 1 && tp->tm_hour <= 12], pos);
		strncpy(tmpstr, str, count);
	}
	
	size_t ret = strftime(str, count, tmpstr, tp);
	free(tmpstr);
	return ret;
}

// from https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/
size_t b64_encoded_size(size_t inlen) {
	size_t ret;

	ret = inlen;
	if (inlen % 3 != 0)
		ret += 3 - (inlen % 3);
	ret /= 3;
	ret *= 4;

	return ret;
}

char* b64encode(u8* in, size_t len) {
	const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

	if (in == NULL || len == 0) return NULL;

	size_t elen = b64_encoded_size(len);
	char* out = malloc(elen + 1);
	if (!out) {
		_e_errno();
		return NULL;
	}
	out[elen] = '\0';

	for (size_t i = 0, j = 0; i < len; i += 3, j += 4) {
		size_t v = in[i];
		v = in[i];
		v = i+1 < len ? v << 8 | in[i+1] : v << 8;
		v = i+2 < len ? v << 8 | in[i+2] : v << 8;

		out[j]   = b64chars[(v >> 18) & 0x3F];
		out[j+1] = b64chars[(v >> 12) & 0x3F];
		if (i+1 < len) {
			out[j+2] = b64chars[(v >> 6) & 0x3F];
		} else {
			out[j+2] = '\0';
		}
		if (i+2 < len) {
			out[j+3] = b64chars[v & 0x3F];
		} else {
			out[j+3] = '\0';
		}
	}
	return out;
}



// from https://stackoverflow.com/a/2256974
int rmdir_r(char *path) {
	DIR *d = opendir(path);
	size_t path_len = strlen(path);
	int r = -1;

	if (d) {
		struct dirent *p;

		r = 0;
		while (!r && (p=readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;

			/* Skip the names "." and ".." as we don't want to recurse on them. */
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
				continue;

			len = path_len + strlen(p->d_name) + 2;
			buf = malloc(len);

			if (buf) {
				struct stat statbuf;

				snprintf(buf, len, "%s/%s", path, p->d_name);
				if (!stat(buf, &statbuf)) {
					if (S_ISDIR(statbuf.st_mode)) {
						r2 = rmdir_r(buf);
					} else {
						r2 = unlink(buf);
					}
				}
				free(buf);
			}
			r = r2;
		}
		closedir(d);
	}

	if (!r) {
		r = rmdir(path);
	}

	return r;
}

void mkdir_p(const char* orig_path) {
	int maxlen = strlen(orig_path) + 1;
	char path[maxlen];
	memcpy(path, orig_path, maxlen);
	path[maxlen - 1] = 0;
	int pos = 0;
	do {
		char* found = strchr(path + pos + 1, '/');
		if (!found) {
			break;
		}
		*found = '\0';
		mkdir(path, 777);
		*found = '/';
		pos = (int)found - (int)path;
	} while(pos < maxlen);
}

int cp(const char* from_path, const char* to_path) {
	FILE* from = 0;
	FILE* to = 0;
	void* buf = 0;
	from = fopen(from_path, "rb");
	if (!from) goto fail;
	to = fopen(to_path, "wb");
	if (!to) goto fail;
	buf = malloc(1000);
	if (!buf) goto fail;
	fseek(from, 0, SEEK_END);
	size_t file_size = ftell(from);
	fseek(from, 0, SEEK_SET);
	while (file_size > 0) {
		size_t write_size = file_size < 1000 ? file_size : 1000;
		
		fread_blk(buf, write_size, 1, from);
		fwrite_blk(buf, write_size, 1, to);
		
		file_size -= write_size;
	}
	free(buf);
	fclose(from);
	fclose(to);
	return 0;
fail:;
	int saved_errno = errno;
	if (buf) free(buf);
	if (from) fclose(from);
	if (to) fclose(to);
	errno = saved_errno;
	return -1;
}

// cppcheck-suppress unusedFunction
Result APT_Wrap(u32 in_size, void* in, u32 nonce_offset, u32 nonce_size, u32 out_size, void* out) {
	u32 cmdbuf[16];
	cmdbuf[0] = IPC_MakeHeader(0x46, 4, 4); // 0x001F0084
	cmdbuf[1] = out_size;
	cmdbuf[2] = in_size;
	cmdbuf[3] = nonce_offset;
	cmdbuf[4] = nonce_size;

	cmdbuf[5] = IPC_Desc_Buffer(in_size, IPC_BUFFER_R);
	cmdbuf[6] = (u32)in;
	cmdbuf[7] = IPC_Desc_Buffer(out_size, IPC_BUFFER_W);
	cmdbuf[8] = (u32)out;

	Result res = aptSendCommand(cmdbuf);
	if (R_FAILED(res)) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result APT_Unwrap(u32 in_size, void* in, u32 nonce_offset, u32 nonce_size, u32 out_size, void* out) {
	u32 cmdbuf[16];
	cmdbuf[0] = IPC_MakeHeader(0x47, 4, 4); // 0x001F0084
	cmdbuf[1] = out_size;
	cmdbuf[2] = in_size;
	cmdbuf[3] = nonce_offset;
	cmdbuf[4] = nonce_size;

	cmdbuf[5] = IPC_Desc_Buffer(in_size, IPC_BUFFER_R);
	cmdbuf[6] = (u32)in;
	cmdbuf[7] = IPC_Desc_Buffer(out_size, IPC_BUFFER_W);
	cmdbuf[8] = (u32)out;

	Result res = aptSendCommand(cmdbuf);
	if (R_FAILED(res)) return res;
	res = (Result)cmdbuf[1];

	return res;
}

// From libctru https://github.com/devkitPro/libctru/blob/faf5162b60eab5402d3839330f985b84382df76c/libctru/source/applets/miiselector.c#L153
u16 crc16_ccitt(void const *buf, size_t len, uint32_t starting_val) {
	if (!buf)
		return -1;

	u8 const *cbuf = buf;
	u32 crc = starting_val;

	static const u16 POLY = 0x1021;

	for (size_t i = 0; i < len; i++)
	{
		for (int bit = 7; bit >= 0; bit--)
			crc = ((crc << 1) | ((cbuf[i] >> bit) & 0x1)) ^ (crc & 0x8000 ? POLY : 0);
	}

	for (int _ = 0; _ < 16; _++)
		crc = (crc << 1) ^ (crc & 0x8000 ? POLY : 0);

	return (u16)(crc & 0xffff);
}

Result decryptMii(void* data, MiiData* mii) {
	Result res = 0;
	MiiData* out = malloc(sizeof(MiiData) + 4);
	if (!out) {
		res = _e(ERROR_OUT_OF_MEMORY);
		goto error;
	}
	res = APT_Unwrap(0x70, data, 12, 10, sizeof(MiiData) + 4, out);
	if (R_FAILED(res)) goto error;
	if (out->version != 0x03) {
		res = ERROR_INVALID_MII;
		goto error;
	}

	u16 crc_calc = crc16_ccitt(out, sizeof(MiiData) + 2, 0);
	u16 crc_check = __builtin_bswap16(*(u16*)(((u8*)out) + sizeof(MiiData) + 2));
	if (crc_calc != crc_check) {
		res = ERROR_INVALID_MII;
		goto error;
	}

	memcpy(mii, out, sizeof(MiiData));

error:
	free(out);
	return res;
}

u8* memsearch(u8* buf, size_t buf_len, u8* cmp, size_t cmp_len) {
	u8* buf_orig = buf;
	while (buf_len - ((int)(buf - buf_orig)) > 0 && (buf = memchr(buf, *(uint8_t*)cmp, buf_len - ((int)(buf - buf_orig))))) {
		if (memcmp(buf, cmp, cmp_len) == 0) {
			return buf;
		}
		buf++;
	}
	return NULL;
}

// from https://github.com/joel16/3DShell/blob/b0c6c9e6a779957b5fb9caf4d6d9cfe3acb4ff92/source/textures.cpp#L150
u32 GetNextPowerOf2(u32 v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return (v >= 64 ? v : 64);
}
bool rgbToImage(C2D_Image* img, u32 width, u32 height, u8* buf) {
	if (width >= 1024 || height >= 1024) return false;

	C3D_Tex* tex = malloc(sizeof(C3D_Tex));
	if (!tex) {
		_e(ERROR_OUT_OF_MEMORY);
		return false;
	}
	memset(tex, 0, sizeof(C3D_Tex));
	Tex3DS_SubTexture* subtex = malloc(sizeof(Tex3DS_SubTexture));
	if (!subtex) {
		free(tex);
		return false;
	}
	memset(subtex, 0, sizeof(Tex3DS_SubTexture));
	subtex->width = (u16)width;
	subtex->height = (u16)height;

	u32 w_pow2 = GetNextPowerOf2(width);
	u32 h_pow2 = GetNextPowerOf2(height);

	subtex->left = 0.0f;
	subtex->top = 1.0f;
	subtex->right = 1.0f * width / w_pow2;
	subtex->bottom = 1.0 - (1.0f * subtex->height / h_pow2);

	C3D_TexInit(tex, (u16)w_pow2, (u16)h_pow2, GPU_RGBA8);
	C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);
	memset(tex->data, 0, tex->size);

	for (u32 x = 0; x < width; x++) {
		for (u32 y = 0; y < height; y++) {
			u32 dst_pos = ((((y >> 3) * (w_pow2 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) * 4;
			u32 src_pos = (y * width + x) * 3;
			// RGBA -> ABGR
			u8 pxl[4];
			pxl[3] = buf[src_pos + 0];
			pxl[2] = buf[src_pos + 1];
			pxl[1] = buf[src_pos + 2];
			pxl[0] = 0xFF;
			memcpy(&((u8*)tex->data)[dst_pos], &pxl, 4);
		}
	}

	C3D_TexFlush(tex);
	tex->border = 0xFFFFFFFF; // transparent
	C3D_TexSetWrap(tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);

	img->tex = tex;
	img->subtex = subtex;
	return true;
}

void C2D_ImageDelete(C2D_Image* img) {
	C3D_TexDelete(img->tex);
	free(img->tex);
	free((void*)img->subtex);
}

bool loadJpeg(C2D_Image* img, u8* data, u32 size) {
	njInit();
	if (njDecode(data, size)) {
		njDone();
		return false;
	}
	int width = njGetWidth();
	int height = njGetHeight();
	bool success = rgbToImage(img, width, height, njGetImage());
	njDone();
	return success;
}

bool loadFileJpeg(C2D_Image* img, const char* filename) {
	FILE* f = fopen(filename, "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	u8* buf = malloc(size);
	if (!buf) {
		fclose(f);
		_e(ERROR_OUT_OF_MEMORY);
		return false;
	}
	fread_blk(buf, size, 1, f);
	bool success = loadJpeg(img, buf, size);
	free(buf);
	fclose(f);
	return success;
}

bool loadFilePng(C2D_Image* img, const char* filename) {
	u32 width;
	u32 height;
	u8* buf;
	if (lodepng_decode24_file(&buf, &width, &height, filename)) {
		return false;
	}
	bool success = rgbToImage(img, width, height, buf);
	free(buf);
	return success;
}

size_t fread_blk(void* buffer, size_t size, size_t count, FILE* stream) {
	size_t total_read = 0;
	u8* buf = (u8*)buffer;
	for (size_t i = 0; i < count; i++) {
		size_t want_read = size;
		while (want_read > 0) {
			svcSleepThread(100);
			size_t read_size = want_read > 512 ? 512 : want_read;
			if (!fread(buf, read_size, 1, stream)) break;
			want_read -= read_size;
			total_read += read_size;
			buf += read_size;
		}
	}
	return total_read / size;
}

size_t fwrite_blk(void* buffer, size_t size, size_t nmemb, FILE* stream) {
	size_t total_write = 0;
	u8* buf = (u8*)buffer;
	for (size_t i = 0; i < nmemb; i++) {
		size_t want_write = size;
		while (want_write > 0) {
			svcSleepThread(100);
			size_t write_size = want_write > 512 ? 512 : want_write;
			if (!fwrite(buf, write_size, 1, stream)) break;
			want_write -= write_size;
			total_write += write_size;
			buf += write_size;
		}
	}
	return total_write / size;
}

char* fgets_blk(char* str, int num, FILE* stream) {
	char* buf = str;
	int want_size = num;
	while (want_size > 0) {
		svcSleepThread(100);
		int read_size = want_size > 512 ? 512 : want_size;
		if (!fgets(buf, read_size, stream)) return NULL;
		want_size -= read_size;
		buf += read_size;
	}
	return str;
}

int fputs_blk(const char* str, FILE* stream) {
	return fwrite_blk((void*)str, strlen(str), 1, stream);
}

void open_url(char* url) {
	if (!url) {
		aptLaunchSystemApplet(APPID_WEB, 0, 0, 0);
		return;
	}
	size_t url_len = strlen(url) + 1;
	if (url_len > 0x400) return open_url(NULL);
	size_t buffer_size = url_len + 1;
	u8* buffer = malloc(buffer_size);
	if (!buffer) return open_url(NULL);
	memcpy(buffer, url, url_len);
	buffer[url_len] = 0;
	aptLaunchSystemApplet(APPID_WEB, buffer, buffer_size, 0);
	free(buffer);
}

// from libctru: https://github.com/devkitPro/libctru/blob/master/libctru/source/os-versionbin.c#L36
static Result osReadVersionBin(u64 tid, OS_VersionBin *versionbin) {
	Result ret = romfsMountFromTitle(tid, MEDIATYPE_NAND, "ver");
	if (R_FAILED(ret))
		return ret;

	FILE* f = fopen("ver:/version.bin", "r");
	if (!f) {
		ret = MAKERESULT(RL_PERMANENT, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
	} else {
		if (fread(versionbin, 1, sizeof(OS_VersionBin), f) != sizeof(OS_VersionBin)) {
			ret = MAKERESULT(RL_PERMANENT, RS_INVALIDSTATE, RM_APPLICATION, RD_NO_DATA);
		}
		fclose(f);
	}

	romfsUnmount("ver");
	return ret;
}

Result get_os_version(OS_VersionBin* ver) {
	#define TID_HIGH 0x000400DB00000000ULL
	static const u32 __CVer_tidlow_regionarray[7] = {
		0x00017202, //JPN
		0x00017302, //USA
		0x00017102, //EUR
		0x00017202, //"AUS"
		0x00017402, //CHN
		0x00017502, //KOR
		0x00017602, //TWN
	};
	
	static const u8 __map_to_try[6] = {
		2, 1, 4, 6, 5, 0,
	};
	
	Result res = cfguInit();
	if (R_SUCCEEDED(res)) {
		u8 region = 0;
		res = CFGU_SecureInfoGetRegion(&region);
		if (R_SUCCEEDED(res) && region < 7) {
			res = osReadVersionBin(TID_HIGH | __CVer_tidlow_regionarray[region], ver);
			if (R_SUCCEEDED(res)) return res;
		}
		cfguExit();
	}

	for (int region = 0; region < 6; region++) {
		res = osReadVersionBin(TID_HIGH | __CVer_tidlow_regionarray[__map_to_try[region]], ver);
		if (R_SUCCEEDED(res)) break;
	}
	
	return res;
}

ErrorData current_errdata = {0};

Result __e(Result error, const char* func, const char* file, const int line) {
	if (R_FAILED(current_errdata.error)) {
		// do nothing if already set
		return error;
	}
	if (R_FAILED(error)) {
		current_errdata.error = error;
		current_errdata.func = func;
		current_errdata.file = file;
		current_errdata.line = line;
	}
	return error;
}

Result __e_errno(const char* func, const char* file, const int line) {
	int eno = errno;
	if (!eno) {
		return 0;
	}
	if (current_errdata.std_errno) {
		// do nothing if already set
		return ERROR_ERRNO;
	}
	current_errdata.std_errno = eno;
	current_errdata.error = ERROR_ERRNO;
	current_errdata.func = func;
	current_errdata.file = file;
	current_errdata.line = line;
	return ERROR_ERRNO;
}

Scene* get_new_error_scene(void) {
	if (current_errdata.error || current_errdata.std_errno) {
		Scene* scene = getErrorScene(&current_errdata);
		current_errdata.error = 0;
		current_errdata.std_errno = 0;
		return scene;
	}
	return NULL;
}

const char* error_desc_str_map[] = {
	"NoTitleId",
	"MissingSlotMeta",
	"UnkTitleId",
	"DupMsg",
	"InvMsg",
	"BoxFull",
	"CurlNoHandle",
	"BadIntegrationList",
	"NoToken",
	"NoPassUrl",
	"BadReportList",
	"InvalidMii",
	"Errno",
	"BadCecdState",
	"NoStreetpassGames",
	"SameLocationTwice",
	"InvalidServerResp",
	"InvalidLocation",
	"InvalidQrPayload",
	"MusicNotInited",
};

const char* error_desc_desc_map[] = {
	"The specified title id is 0, making it invalid.",
	"No metadata for the slot to be processed was provided.",
	"The provided title id is not known to this system.",
	"There is a duplicate message. This likely means that you tried to add a message that already exists.",
	"The message did not pass validation, making it invalid.",
	"The box where a message should be added is already full.",
	"There is no free cURL handle currently, making it impossible to do this network call.",
	"The integration list provided from the server is malformed.",
	"The verification QR code lacks a token.",
	"The download pass QR code lacks an http url.",
	"The report list stored on the SD card is malformed.",
	"The mii data is invalid.",
	"See errno",
	"The internal state of the CECD service is incorrect. Try closing NetPass and opening it again.",
	"There are no games with StreetPass enabled. Please enable StreetPass in at least one title and open NetPass again.",
	"You can't enter the same location twice in a row.",
	"The server responded with an invalid response. Try closing NetPass and opening it again.",
	"The location is invalid.",
	"The QR code you tried to scan is invalid / malformed.",
	"The music module has not been initialised yet.",
};

void miscInit(void) {
	set_application_desc_map(sizeof(error_desc_str_map) / sizeof(error_desc_str_map[0]), error_desc_str_map, error_desc_desc_map);
}

void cecTimeToTm(CecTimestamp* cec, struct tm* tm) {
	tm->tm_year = cec->year - 1900;
	tm->tm_mon = cec->month == 0 || cec->month > 12 ? 0 : cec->month - 1;
	tm->tm_mday = cec->day == 0 || cec->day > 31 ? 0 : cec->day;
	tm->tm_wday = cec->weekday > 6 ? 0 : cec->weekday;
	tm->tm_hour = cec->hour > 23 ? 0 : cec->hour;
	tm->tm_min = cec->minute > 59 ? 0 : cec->minute;
	tm->tm_sec = cec->second > 59 ? 0 : cec->second;
	tm->tm_isdst = false;
}

int format_uuid(char str[37], u8 uuid[16]) {
	return snprintf(str, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		uuid[0], uuid[1], uuid[2], uuid[3],
		uuid[4], uuid[5],
		uuid[6], uuid[7],
		uuid[8], uuid[9],
		uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]
	);
}

// decompression code stolen from ctrtool
u32 blz_decompress_size(u8* compressed, u32 compressedsize) {
	return compressedsize + *(u32*)(compressed + compressedsize - 4);
}
bool blz_decompress(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize) {
	u8* footer = compressed + compressedsize - 8;
	u32 buffertopandbottom = (footer[0]<<0) | (footer[1]<<8) | (footer[2]<<16) | (footer[3]<<24);
	u32 i, j;
	u32 out = decompressedsize;
	u32 index = compressedsize - ((buffertopandbottom>>24)&0xFF);
	u32 segmentoffset;
	u32 segmentsize;
	u8 control;
	u32 stopindex = compressedsize - (buffertopandbottom&0xFFFFFF);

	memset(decompressed, 0, decompressedsize);
	memcpy(decompressed, compressed, compressedsize);

	while(index > stopindex) {
		control = compressed[--index];
		for(i=0; i<8; i++) {
			if (index <= stopindex) break;
			if (index <= 0) break;
			if (out <= 0) break;
			if (control & 0x80) {
				// compression out of bounds
				if (index < 2) goto clean;
				index -= 2;
				segmentoffset = compressed[index] | (compressed[index+1]<<8);
				segmentsize = ((segmentoffset >> 12)&15)+3;
				segmentoffset &= 0x0FFF;
				segmentoffset += 2;
				// compression out of bounds
				if (out < segmentsize) goto clean;
				for(j=0; j<segmentsize; j++) {
					u8 data;
					// compression out of bounds
					if (out+segmentoffset >= decompressedsize) goto clean;
					data  = decompressed[out+segmentoffset];
					decompressed[--out] = data;
				}
			} else {
				// compression out of bounds
				if (out < 1) goto clean;
				decompressed[--out] = compressed[--index];
			}
			control <<= 1;
		}
	}
	return true;
	
	clean:
	return false;
}

/* Code borrowed from GodMode9i:
	https://github.com/DS-Homebrew/GodMode9i/blob/d68ac105e68b4a1fc2c706a08c7a394255c325c2/arm9/source/driveOperations.cpp#L166-L170
*/
u64 getAvailableSpace(void) {
	struct statvfs st;
	statvfs("sdmc:/", &st);
	return (u64)st.f_bsize * (u64)st.f_bavail;
}

Result get_cia_info(const char* cia_filename, AM_TitleEntry* info) {
	Result res = 0;
	
	char* real_filename = strchr(cia_filename, ':');
	if (real_filename) {
		real_filename++;
	} else {
		real_filename = (char*)cia_filename;
	}
	
	Handle file_handle;
	
	res = FSUSER_OpenFileDirectly(&file_handle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, real_filename), FS_OPEN_READ, 0);
	if (R_FAILED(res)) return res;
	res = AM_GetCiaFileInfo(MEDIATYPE_SD, info, file_handle);
	FSFILE_Close(file_handle);
	return res;
}

// strongly inspired from universal updater code
FS_MediaType get_title_destination(u64 title_id) {
	u16 platform = (u16) ((title_id >> 48) & 0xFFFF);
	u16 category = (u16) ((title_id >> 32) & 0xFFFF);
	u8 variation = (u8) (title_id & 0xFF);

	//     DSiWare                3DS                    DSiWare, System, DLP         Application           System Title
	return platform == 0x0003 || (platform == 0x0004 && ((category & 0x8011) != 0 || (category == 0x0000 && variation == 0x02))) ? MEDIATYPE_NAND : MEDIATYPE_SD;
}
Result install_cia(const char* cia_filename) {
	Result res = 0;
	Handle cia_handle, file_handle;
	AM_TitleEntry info;
	FS_MediaType media = MEDIATYPE_SD;
	u64 file_size;
	
	char* real_filename = strchr(cia_filename, ':');
	if (real_filename) {
		real_filename++;
	} else {
		real_filename = (char*)cia_filename;
	}
	
	res = _e(FSUSER_OpenFileDirectly(&file_handle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, real_filename), FS_OPEN_READ, 0));
	if (R_FAILED(res)) return res;
	
	res = _e(AM_GetCiaFileInfo(media, &info, file_handle));
	if (R_FAILED(res)) {
		FSFILE_Close(file_handle);
		return res;
	}
	
	media = get_title_destination(info.titleID);
	
	res = _e(FSFILE_GetSize(file_handle, &file_size));
	if (R_FAILED(res)) {
		FSFILE_Close(file_handle);
		return res;
	}
	if (file_size > getAvailableSpace()) {
		logln(ERROR, "wtf?!");
		logln(ERROR, "%lld, %lld", file_size, getAvailableSpace());
		res = -1; // TODO: proper error
		FSFILE_Close(file_handle);
		return res;
	}
	
	res = _e(AM_StartCiaInstall(media, &cia_handle));
	if (R_FAILED(res)) {
		FSFILE_Close(file_handle);
		return res;
	}
	
	u32 to_read = 0x200000;
	u8 *buf = malloc(to_read);
	if (!buf) {
		res = _e(ERROR_OUT_OF_MEMORY);
		FSFILE_Close(file_handle);
		return res;
	}
	
	u32 install_size = file_size;
	u32 install_offset = 0;
	u32 bytes_read, bytes_written;
	do {
		res = _e(FSFILE_Read(file_handle, &bytes_read, install_offset, buf, to_read));
		if (R_FAILED(res)) {
			free(buf);
			FSFILE_Close(file_handle);
			return res;
		}
		res = _e(FSFILE_Write(cia_handle, &bytes_written, install_offset, buf, to_read, FS_WRITE_FLUSH));
		if (R_FAILED(res)) {
			free(buf);
			FSFILE_Close(file_handle);
			return res;
		}
		install_offset += bytes_read;
	} while(install_offset < install_size);
	free(buf);
	FSFILE_Close(file_handle);
	
	res = _e(AM_FinishCiaInstall(cia_handle));
	if (R_FAILED(res)) return res;

	return res;
}
