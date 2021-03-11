/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "Log.h"
#import <asl.h>
#import <os/log.h>

#ifdef USE_CRASHLYTICS
@import FirebaseCrashlytics;
#endif

@implementation Log

#define FILE_SIZE 17
#define DOMAIN_SIZE 3


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
	linphone_core_set_log_collection_path([LinphoneManager cacheDirectory].UTF8String);
	linphone_core_set_log_handler(linphone_iphone_log_handler);
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

+ (void)directLog:(OrtpLogLevel)level text:(NSString *)text {
	bctbx_log(BCTBX_LOG_DOMAIN, level, "%s", [text cStringUsingEncoding:NSUTF8StringEncoding]);
}

#pragma mark - Logs Functions callbacks

void linphone_iphone_log_handler(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
	NSString *format = [[NSString alloc] initWithUTF8String:fmt];
	NSString *formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	NSString *lvl;

	if (!domain)
		domain = "lib";
	// since \r are interpreted like \n, avoid double new lines when logging network packets (belle-sip)
	// output format is like: I/ios/some logs. We truncate domain to **exactly** DOMAIN_SIZE characters to have
	// fixed-length aligned logs
	switch (lev) {
		case ORTP_FATAL:
			lvl = @"Fatal";
			break;
		case ORTP_ERROR:
			lvl = @"Error";
			break;
		case ORTP_WARNING:
			lvl = @"Warning";
			break;
		case ORTP_MESSAGE:
			lvl = @"Message";
			break;
		case ORTP_DEBUG:
			lvl = @"Debug";
			break;
		case ORTP_TRACE:
			lvl = @"Trace";
			break;
		case ORTP_LOGLEV_END:
			return;
	}
	if ([formatedString containsString:@"\n"]) {
		NSArray *myWords = [[formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]
			componentsSeparatedByString:@"\n"];
		for (int i = 0; i < myWords.count; i++) {
			NSString *tab = i > 0 ? @"\t" : @"";
			if (((NSString *)myWords[i]).length > 0) {
#ifdef USE_CRASHLYTICS
			[[FIRCrashlytics crashlytics] logWithFormat:@"[%@] %@%@", lvl, tab, (NSString *)myWords[i]];
#endif
			NSLog(@"[%@] %@%@", lvl, tab, (NSString *)myWords[i]);
			}
		}
	} else {
#ifdef USE_CRASHLYTICS
		[[FIRCrashlytics crashlytics] logWithFormat:@"[%@] %@", lvl, [formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]];
#endif
		NSLog(@"[%@] %@", lvl, [formatedString stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]);
	}
}

@end
