/**
 * NetPass
 * Copyright (C) 2025 yabobay
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

void logln(const char *restrict format, ...) {
	va_list args;
	va_start(args, format);
	switch (config.log_output) {
	case BottomScreen:
		printf("[LOG] ");
		vprintf(format, args);
		putchar('\n');
		break;
	default:
		printf("[TODO]: Logging method %d\n", config.log_output);
		break;
	}
	va_end(args);
}

LogMessage* log_start(void) {
	LogMessage* log = malloc(sizeof(LogMessage));
	if (!log) {
		_e(ERROR_OUT_OF_MEMORY);
		return NULL;
	}
	log->length = 0;
	log->message = NULL;
	return log;
}

void log_multi(LogMessage* log, const char *restrict format, ...) {
	va_list args;
	va_start(args, format);
	int buf_length = 10;
	char *buf = malloc(buf_length);
	if (!buf) {
		_e(ERROR_OUT_OF_MEMORY);
		return;
	}
try_write:
	int written = vsnprintf(buf, buf_length, format, args);
	if (written >= buf_length) {
		buf_length = written + 1;
		char *tmp = realloc(buf, buf_length);
		if (!tmp) {
			free(buf);
			_e(ERROR_OUT_OF_MEMORY);
			return;
		}
		buf = tmp;
		goto try_write;
	}
	log->length += buf_length;
	char *tmp = realloc(log->message, log->length);
	if (!tmp) {
		free(log->message);
		_e(ERROR_OUT_OF_MEMORY);
		return;
	}
	log->message = tmp;
	va_end(args);
	strcat(log->message, buf);
	free(buf);
}

void log_end(LogMessage* log) {
	logln(log->message);
	free(log->message);
	free(log);
}
