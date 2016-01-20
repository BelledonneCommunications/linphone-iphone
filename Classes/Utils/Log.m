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

#define FILESIZE 17
#define DOMAIN_SIZE 3

+ (NSString *)cacheDirectory {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString *cachePath = [paths objectAtIndex:0];
	BOOL isDir = NO;
	NSError *error;
	// cache directory must be created if not existing
	if (![[NSFileManager defaultManager] fileExistsAtPath:cachePath isDirectory:&isDir] && isDir == NO) {
		[[NSFileManager defaultManager] createDirectoryAtPath:cachePath
								  withIntermediateDirectories:NO
												   attributes:nil
														error:&error];
	}
	return cachePath;
}

+ (void)log:(OrtpLogLevel)severity file:(const char *)file line:(int)line format:(NSString *)format, ... {
	va_list args;
	va_start(args, format);
	NSString *str = [[NSString alloc] initWithFormat:format arguments:args];
	const char *utf8str = [str cStringUsingEncoding:NSString.defaultCStringEncoding];
	const char *filename = strchr(file, '/') ? strrchr(file, '/') + 1 : file;

	if ((severity & ORTP_DEBUG) != 0) {
		// lol: ortp_debug(XXX) can be disabled at compile time, but ortp_log(ORTP_DEBUG, xxx) will always be valid even
		//      not in debug build...
		ortp_debug("%*s:%-4d/%s", FILESIZE, filename + MAX((int)strlen(filename) - FILESIZE, 0), line, utf8str);
	} else {
		ortp_log(severity, "%*s:%-4d/%s", FILESIZE, filename + MAX((int)strlen(filename) - FILESIZE, 0), line, utf8str);
	}
	va_end(args);
}

+ (void)enableLogs:(BOOL)enabled {
	linphone_core_set_log_collection_path([self cacheDirectory].UTF8String);
	linphone_core_enable_log_collection(enabled);
	linphone_core_enable_logs_with_cb(linphone_iphone_log_handler);
	if (enabled) {
		NSLog(@"Enabling debug logs");
		linphone_core_set_log_level(ORTP_DEBUG);
	} else {
		NSLog(@"Disabling debug logs");
		linphone_core_set_log_level(ORTP_ERROR);
	}
	ortp_set_log_level_mask("ios", ORTP_DEBUG | ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR | ORTP_FATAL);
}

#pragma mark - Logs Functions callbacks

void linphone_iphone_log_handler(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
	NSString *format = [[NSString alloc] initWithUTF8String:fmt];
	NSString *formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	NSString *lvl = @"";
	if ((lev & ORTP_FATAL) != 0) {
		lvl = @"F";
	} else if ((lev & ORTP_ERROR) != 0) {
		lvl = @"E";
	} else if ((lev & ORTP_WARNING) != 0) {
		lvl = @"W";
	} else if ((lev & ORTP_MESSAGE) != 0) {
		lvl = @"I";
	} else if (((lev & ORTP_TRACE) != 0) || ((lev & ORTP_DEBUG) != 0)) {
		lvl = @"D";
	}
	if (!domain)
		domain = "liblinphone";
	// since \r are interpreted like \n, avoid double new lines when logging network packets (belle-sip)
	// output format is like: I/ios/some logs. We truncate domain to **exactly** DOMAIN_SIZE characters to have
	// fixed-length aligned logs
	NSLog(@"%@/%*.*s/%@", lvl, DOMAIN_SIZE, DOMAIN_SIZE, domain,
		  [formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]);
}

@end
