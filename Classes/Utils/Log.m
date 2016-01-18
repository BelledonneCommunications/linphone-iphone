/*
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "Log.h"

@implementation Log

+ (void)log:(OrtpLogLevel)severity file:(const char *)file line:(int)line format:(NSString *)format, ... {
	va_list args;
	va_start(args, format);
	NSString *str = [[NSString alloc] initWithFormat:format arguments:args];
	const char *utf8str = [str cStringUsingEncoding:NSString.defaultCStringEncoding];
	int filesize = 20;
	const char *filename = strchr(file, '/') ? strrchr(file, '/') + 1 : file;

	char levelC = 'U'; // undefined
	if ((severity & ORTP_FATAL) != 0) {
		levelC = 'F';
	} else if ((severity & ORTP_ERROR) != 0) {
		levelC = 'E';
	} else if ((severity & ORTP_WARNING) != 0) {
		levelC = 'W';
	} else if ((severity & ORTP_MESSAGE) != 0) {
		levelC = 'I';
	} else if ((severity & ORTP_DEBUG) != 0) {
		levelC = 'D';
	}

	if ((severity & ORTP_DEBUG) != 0) {
		// lol: ortp_debug(XXX) can be disabled at compile time, but ortp_log(ORTP_DEBUG, xxx) will always be valid even
		//      not in debug build...
		ortp_debug("%c %*s:%3d - %s", levelC, filesize, filename + MAX((int)strlen(filename) - filesize, 0), line,
				   utf8str);
	} else {
		// we want application logs to be always enabled (except debug ones) so use | ORTP_ERROR extra mask
		ortp_log(severity | ORTP_ERROR, "%c %*s:%3d - %s", levelC, filesize,
				 filename + MAX((int)strlen(filename) - filesize, 0), line, utf8str);
	}
	va_end(args);
}

+ (void)enableLogs:(BOOL)enabled {
	linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
	if (enabled) {
		NSLog(@"Enabling debug logs");
		linphone_core_set_log_level(ORTP_DEBUG);
	} else {
		NSLog(@"Disabling debug logs");
		linphone_core_set_log_level(ORTP_ERROR);
	}
	linphone_core_enable_log_collection(enabled);
}

#pragma mark - Logs Functions callbacks

void linphone_iphone_log_handler(int lev, const char *fmt, va_list args) {
	NSString *format = [[NSString alloc] initWithUTF8String:fmt];
	NSString *formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	NSString *lvl = @"";
	if ((lev & APP_LVL) == 0) {
		if ((lev & ORTP_FATAL) != 0) {
			lvl = @"F ";
		} else if ((lev & ORTP_ERROR) != 0) {
			lvl = @"E ";
		} else if ((lev & ORTP_WARNING) != 0) {
			lvl = @"W ";
		} else if ((lev & ORTP_MESSAGE) != 0) {
			lvl = @"I ";
		} else if (((lev & ORTP_TRACE) != 0) || ((lev & ORTP_DEBUG) != 0)) {
			lvl = @"D ";
		}
	}
	// since \r are interpreted like \n, avoid double new lines when logging network packets (belle-sip)
	NSLog(@"%@%@", lvl, [formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]);
}

@end
