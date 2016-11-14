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
#import <asl.h>
#import <os/log.h>

@implementation Log

#define FILE_SIZE 17
#define DOMAIN_SIZE 3

+ (NSString *)cacheDirectory {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString *cachePath = [paths objectAtIndex:0];
	BOOL isDir = NO;
	NSError *error;
	// cache directory must be created if not existing
	if (![[NSFileManager defaultManager] fileExistsAtPath:cachePath isDirectory:&isDir] && isDir == NO) {
		if (![[NSFileManager defaultManager] createDirectoryAtPath:cachePath
									   withIntermediateDirectories:NO
														attributes:nil
															 error:&error]) {
			LOGE(@"Could not create cache directory: %@", error);
		}
	}
	return cachePath;
}

+ (void)log:(OrtpLogLevel)severity file:(const char *)file line:(int)line format:(NSString *)format, ... {
	va_list args;
	va_start(args, format);
	NSString *str = [[NSString alloc] initWithFormat:format arguments:args];
	const char *utf8str = [str cStringUsingEncoding:NSString.defaultCStringEncoding];
	const char *filename = strchr(file, '/') ? strrchr(file, '/') + 1 : file;
	ortp_log(severity, "(%*s:%-4d) %s", FILE_SIZE, filename + MAX((int)strlen(filename) - FILE_SIZE, 0), line, utf8str);
	va_end(args);
}

+ (void)enableLogs:(OrtpLogLevel)level {
	BOOL enabled = (level >= ORTP_DEBUG && level < ORTP_ERROR);
	static BOOL stderrInUse = NO;
	if (!stderrInUse) {
		asl_add_log_file(NULL, STDERR_FILENO);
		stderrInUse = YES;
	}
	linphone_core_set_log_collection_path([self cacheDirectory].UTF8String);
	linphone_core_enable_logs_with_cb(linphone_iphone_log_handler);
	linphone_core_enable_log_collection(enabled);
	if (level == 0) {
		linphone_core_set_log_level(ORTP_FATAL);
		ortp_set_log_level("ios", ORTP_FATAL);
		NSLog(@"I/%s/Disabling all logs", ORTP_LOG_DOMAIN);
	} else {
		NSLog(@"I/%s/Enabling %s logs", ORTP_LOG_DOMAIN, (enabled ? "all" : "application only"));
		linphone_core_set_log_level(level);
		ortp_set_log_level("ios", level == ORTP_DEBUG ? ORTP_DEBUG : ORTP_MESSAGE);
	}
}

#pragma mark - Logs Functions callbacks

void linphone_iphone_log_handler(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
	NSString *format = [[NSString alloc] initWithUTF8String:fmt];
	NSString *formatedString = [[NSString alloc] initWithFormat:format arguments:args];

	if (!domain)
		domain = "lib";
	// since \r are interpreted like \n, avoid double new lines when logging network packets (belle-sip)
	// output format is like: I/ios/some logs. We truncate domain to **exactly** DOMAIN_SIZE characters to have
	// fixed-length aligned logs

	if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
		os_log_t log = os_log_create("subsystem", "Notice");
		os_log_type_t type = OS_LOG_TYPE_INFO;
		switch (lev) {
			case ORTP_FATAL:
				type = OS_LOG_TYPE_FAULT;
				log = os_log_create("subsystem", "Fatal");
				break;
			case ORTP_ERROR:
				type = OS_LOG_TYPE_ERROR;
				log = os_log_create("subsystem", "Error");
				break;
			case ORTP_WARNING:
				type = OS_LOG_TYPE_DEFAULT;
				log = os_log_create("subsystem", "Warning");
				break;
			case ORTP_MESSAGE:
				type = OS_LOG_TYPE_INFO;
				log = os_log_create("subsystem", "Notice");
				break;
			case ORTP_DEBUG:
			case ORTP_TRACE:
				type = OS_LOG_TYPE_INFO;
				log = os_log_create("subsystem", "Debug");
				break;
			case ORTP_LOGLEV_END:
				return;
		}
		os_log_with_type(log, type, "%{public}s/%{public}s", domain,
						 [formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"].UTF8String);
	} else {
		int lvl = ASL_LEVEL_NOTICE;
		switch (lev) {
			case ORTP_FATAL:
				lvl = ASL_LEVEL_CRIT;
				break;
			case ORTP_ERROR:
				lvl = ASL_LEVEL_ERR;
				break;
			case ORTP_WARNING:
				lvl = ASL_LEVEL_WARNING;
				break;
			case ORTP_MESSAGE:
				lvl = ASL_LEVEL_NOTICE;
				break;
			case ORTP_DEBUG:
			case ORTP_TRACE:
				lvl = ASL_LEVEL_INFO;
				break;
			case ORTP_LOGLEV_END:
				return;
		}
		asl_log(NULL, NULL, lvl, "%*.*s/%s", DOMAIN_SIZE, DOMAIN_SIZE, domain,
				[formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"].UTF8String);
	}
}

@end
