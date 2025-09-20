/**
 * NetPass
 * Copyright (C) 2024, 2025 Sorunome
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

void logln(const char *restrict string) {
	switch (config.log_output) {
	case BottomScreen:
		printf("[LOG]: %s\n", string);
		break;
	default:
		printf("[TODO]: Logging method %d\n", config.log_output);
		break;
	}
}

LogMessage* log_start(void) {
	LogMessage* log = malloc(sizeof(LogMessage));
	log->length = 0;
	log->message = NULL;
	return log;
}

void log_multi(LogMessage* log, const char *restrict format, ...) {
	va_list args;
	va_start(args, format);
	int tmp_length = 10;
	char *tmp = malloc(tmp_length);
try_write:
	int written = vsnprintf(tmp, tmp_length, format, args);
	if (written >= tmp_length) {
		tmp_length = written + 1;
		tmp = realloc(tmp, tmp_length);
		goto try_write;
	}
	log->length += tmp_length;
	log->message = realloc(log->message, log->length);
	va_end(args);
	strcat(log->message, tmp);
	free(tmp);
}

void log_end(LogMessage* log) {
	logln(log->message);
	free(log->message);
	free(log);
}
