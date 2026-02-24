/**
 * NetPass
 * Copyright (C) 2025 yabobay
 *               2026 Sorunome
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

#include "log.h"
#include "config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

#define LOG_FILE_NAME "sdmc:/config/netpass/log.txt"

static const char* const LOG_LEVEL_NAMES[] = { "ERROR", "WARN", "INFO", "DEBUG" };
static const char* const LOG_LEVEL_COLORS[] = { "34", "36", "35", "31" };

static __FILE* log_file = NULL;

void logInit() {
	if (config.log_output == LogOutputFile) {
		log_file = fopen(LOG_FILE_NAME, "w");
		if (!log_file) {
			_e_errno();
			return;
		}
	} else if (config.log_output == LogOutputBottomScreen) {
		log_file = stdout;
	}
}

void logExit() {
	if (config.log_output == LogOutputFile && log_file) {
		fclose(log_file);
		log_file = NULL;
	}
}

void logln(enum LogLevel level, const char *restrict format, ...) {
	if (!log_file || config.log_level < level) return;
	va_list args;
	va_start(args, format);
	log_line_start(level, "");
	vfprintf(log_file, format, args);
	fputc('\n', log_file);
	fflush(log_file);
	va_end(args);
}

LogMessage* log_start(enum LogLevel level) {
	LogMessage* log = malloc(sizeof(LogMessage));
	if (!log) {
		_e(ERROR_OUT_OF_MEMORY);
		return NULL;
	}
	log->length = 0;
	log->level = level;
	log->message = calloc(1, sizeof(char));
	if (!log->message) {
		_e(ERROR_OUT_OF_MEMORY);
		free(log);
		return NULL;
	}
	return log;
}

void log_multi(LogMessage* log, const char *restrict format, ...) {
	if (!log || !log_file) return;
	va_list args;
	va_start(args, format);
	int buf_length = 10;
	char *buf = malloc(buf_length);
	if (!buf) {
		_e(ERROR_OUT_OF_MEMORY);
		va_end(args);
		return;
	}
	int written;
try_write:
	written = vsnprintf(buf, buf_length, format, args);
	if (written >= buf_length) {
		buf_length = written + 1;
		char *tmp = realloc(buf, buf_length);
		if (!tmp) {
			free(buf);
			_e(ERROR_OUT_OF_MEMORY);
			va_end(args);
			return;
		}
		buf = tmp;
		goto try_write;
	}
	va_end(args);
	log->length += buf_length;
	char *tmp = realloc(log->message, log->length);
	if (!tmp) {
		free(buf);
		_e(ERROR_OUT_OF_MEMORY);
		return;
	}
	log->message = tmp;
	strncat(log->message, buf, log->length);
	free(buf);
}

void log_end(LogMessage* log) {
	if (!log) return;
	logln(log->level, log->message);
	free(log->message);
	free(log);
}

void log_line_start(enum LogLevel level, const char *restrict format, ...) {
	if (!log_file) return;
	va_list args;
	va_start(args, format);
	if (log_file == stdout) {
		fprintf(log_file, "\x1b[%sm[%s]\x1b[0m ", LOG_LEVEL_COLORS[level], LOG_LEVEL_NAMES[level]);
	} else {
		fprintf(log_file, "[%s] ", LOG_LEVEL_NAMES[level]);
	}
	vfprintf(log_file, format, args);
	fflush(log_file);
	va_end(args);
}

void log_line_continue(const char *restrict format, ...) {
	if (!log_file) return;
	va_list args;
	va_start(args, format);
	vfprintf(log_file, format, args);
	fflush(log_file);
	va_end(args);
}

void log_line_finish(const char *restrict format, ...) {
	if (!log_file) return;
	va_list args;
	va_start(args, format);
	fputc(' ', log_file);
	vfprintf(log_file, format, args);
	fputc('\n', log_file);
	fflush(log_file);
	va_end(args);
}
