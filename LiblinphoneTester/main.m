/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
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

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "Log.h"
#import <os/log.h>
#include "linphonetester/liblinphone_tester.h"

#ifdef DEBUG

// Dump exception
void uncaughtExceptionHandler(NSException *exception) {
	NSLog(@"Crash: %@", exception);
	NSLog(@"Stack Trace: %@", [exception callStackSymbols]);
	// Internal error reporting
};

#endif

int main(int argc, char *argv[]) {
#ifdef DEBUG
	NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);
#endif
    int i;
    for(i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--verbose") == 0) {
            linphone_core_set_log_level(ORTP_MESSAGE);
        } else if (strcmp(argv[i],"--log-file")==0){
#if TARGET_OS_SIMULATOR
            char *xmlFile = bc_tester_file("LibLinphoneIOS.xml");
            char *args[] = {"--xml-file", xmlFile};
            bc_tester_parse_args(2, args, 0);
            
            char *logFile = bc_tester_file("LibLinphoneIOS.txt");
            liblinphone_tester_set_log_file(logFile);
#endif
        } else if (strcmp(argv[i],"--no-ipv6")==0){
            liblinphonetester_ipv6 = FALSE;
        }
    }
	@autoreleasepool {
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
