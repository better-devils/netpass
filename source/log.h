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

#pragma once

enum LogOutput {
	BottomScreen = 0,
	File,
	Disabled,
};

enum LogLevel {
	ERROR = 0,
	WARN,
	INFO,
	DEBUG,
};

typedef struct LogMessage {
	int length;
	enum LogLevel level;
	char *message;
} LogMessage;

void logInit();
void logExit();

// print a line on the log with printf syntax
void logln(enum LogLevel level, const char *restrict format, ...);

// compose a message over multiple function calls, and *then* print it
LogMessage* log_start(enum LogLevel level);
void log_multi(LogMessage* foo, const char *restrict format, ...);
void log_end(LogMessage* foo);

// *print* a message bit-by-bit over multiple function calls
void log_line_start(enum LogLevel level, const char *restrict format, ...);
void log_line_continue(const char *restrict format, ...);
void log_line_finish(const char *restrict format, ...);
