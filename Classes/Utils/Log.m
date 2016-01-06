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

@implementation LinphoneLogger

+ (void)log:(OrtpLogLevel)severity file:(const char *)file line:(int)line format:(NSString *)format, ... {
	va_list args;
	va_start(args, format);
	NSString *str = [[NSString alloc] initWithFormat:format arguments:args];
	const char *utf8str = [str cStringUsingEncoding:NSString.defaultCStringEncoding];
	int filesize = 20;
	const char *filename = strchr(file, '/') ? strrchr(file, '/') + 1 : file;
	if (severity <= ORTP_DEBUG) {
		// lol: ortp_debug(XXX) can be disabled at compile time, but ortp_log(ORTP_DEBUG, xxx) will always be valid even
		//      not in debug build...
		ortp_debug("%*s:%3d - %s", filesize, filename + MAX((int)strlen(filename) - filesize, 0), line, utf8str);
	} else {
		ortp_log(severity, "%*s:%3d - %s", filesize, filename + MAX((int)strlen(filename) - filesize, 0), line,
				 utf8str);
	}
	va_end(args);
}

#pragma mark - Logs Functions callbacks

void linphone_iphone_log_handler(int lev, const char *fmt, va_list args) {
	NSString *format = [[NSString alloc] initWithUTF8String:fmt];
	NSString *formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	char levelC = 'I';
	switch ((OrtpLogLevel)lev) {
		case ORTP_FATAL:
			levelC = 'F';
			break;
		case ORTP_ERROR:
			levelC = 'E';
			break;
		case ORTP_WARNING:
			levelC = 'W';
			break;
		case ORTP_MESSAGE:
			levelC = 'I';
			break;
		case ORTP_TRACE:
		case ORTP_DEBUG:
			levelC = 'D';
			break;
		case ORTP_LOGLEV_END:
			return;
	}
	// since \r are interpreted like \n, avoid double new lines when logging packets
	NSLog(@"%c %@", levelC, [formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]);
}

@end
