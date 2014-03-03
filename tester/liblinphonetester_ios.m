/*
 liblinphone_tester - liblinphone test suite
 Copyright (C) 2013  Belledonne Communications SARL

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "liblinphonetester_ios.h"

#include <pthread.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

extern int ios_tester_main(int argc, char * argv[]);

int g_argc;
char** g_argv;

static void* apple_main(void* data) {
    ios_tester_main(g_argc,g_argv);
    return NULL;
}

int apple_start_tests(int argc, char* argv[])
{
    pthread_t main_thread;
	g_argc=argc;
	g_argv=argv;
	return (int)pthread_create(&main_thread,NULL,apple_main,NULL);
}

void liblinphone_tester_ios_log_handler(OrtpLogLevel level, const char* fmt, va_list args)
{
    NSLogv([NSString stringWithUTF8String:fmt], args);
}

#ifdef NO_XCODE /* when compiling under xcode the main is elsewhere */
int main( int argc, char* argv[]){
    return ios_tester_main(argc, argv);
}
#endif