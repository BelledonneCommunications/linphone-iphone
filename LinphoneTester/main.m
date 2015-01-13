//
//  main.m
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import <UIKit/UIKit.h>

#import "AppDelegate.h"
#include <execinfo.h>
#include <signal.h>

static NSString * const kKEY_CRASH_REPORT = @"CRASH_REPORT";
static NSString * const kKEY_ExceptionName = @"UnhandledExceptionName";
static NSString * const kKEY_ExceptionReason = @"UnhandledExceptionReason";
static NSString * const kKEY_ExceptionUserInfo = @"UnhandledExceptionUserInfo";
static NSString * const kKEY_ExceptionCallStack = @"UnhandledExceptionCallStack";
static NSString * const kKEY_ExceptionScreenshot = @"UnhandledExceptionScreenshot";
static NSString * const kKEY_ExceptionTimestamp = @"UnhandledExceptionTimestamp";

static void unhandledExceptionHandler(NSException *exception) {
    NSMutableDictionary *crashReport = [NSMutableDictionary dictionary];
    crashReport[kKEY_ExceptionName] = exception.name;
    crashReport[kKEY_ExceptionReason] = exception.reason;
    crashReport[kKEY_ExceptionUserInfo] = exception.userInfo ?: [NSNull null].debugDescription;
    crashReport[kKEY_ExceptionCallStack] = exception.callStackSymbols.debugDescription;

	NSLog(@"CRASH: %@ - %@", exception.name, exception.callStackSymbols.debugDescription);

    [[NSUserDefaults standardUserDefaults] setObject:[NSDictionary dictionaryWithDictionary:crashReport] forKey:kKEY_CRASH_REPORT];
    [[NSUserDefaults standardUserDefaults] synchronize];
	exit(1);
}

/*	SignalHandler
 *
 *		Handle uncaught signals
 */

void SignalHandler(int sig, siginfo_t *info, void *context)
{
	void *frames[128];
	int i,len = backtrace(frames, 128);
	char **symbols = backtrace_symbols(frames,len);

	/*
	 *	Now format into a message for sending to the user
	 */

	NSMutableString *buffer = [[NSMutableString alloc] initWithCapacity:4096];

	NSBundle *bundle = [NSBundle mainBundle];
	[buffer appendFormat:@"PComp version %@ build %@\n\n",
	 [bundle objectForInfoDictionaryKey:@"CFBundleVersion"],
	 [bundle objectForInfoDictionaryKey:@"CIMBuildNumber"]];
	[buffer appendString:@"Uncaught Signal\n"];
	[buffer appendFormat:@"si_signo    %d\n",info->si_signo];
	[buffer appendFormat:@"si_code     %d\n",info->si_code];
	[buffer appendFormat:@"si_value    %d\n",info->si_value.sival_int];
	[buffer appendFormat:@"si_errno    %d\n",info->si_errno];
	[buffer appendFormat:@"si_addr     %p\n",info->si_addr];
	[buffer appendFormat:@"si_status   %d\n",info->si_status];
	[buffer appendString:@"Stack trace:\n\n"];
	for (i = 0; i < len; ++i) {
		[buffer appendFormat:@"%4d - %s\n",i,symbols[i]];
	}

	/*
	 *	Get the error file to write this to
	 */

	NSLog(@"Error %@",buffer);
	exit(1);
}



void InstallUncaughtExceptionHandler()
{
    NSSetUncaughtExceptionHandler(&unhandledExceptionHandler);
  	struct sigaction mySigAction;
	mySigAction.sa_sigaction = SignalHandler;
	mySigAction.sa_flags = SA_SIGINFO;

	sigemptyset(&mySigAction.sa_mask);
	sigaction(SIGQUIT, &mySigAction, NULL);
	sigaction(SIGILL, &mySigAction, NULL);
	sigaction(SIGTRAP, &mySigAction, NULL);
	sigaction(SIGABRT, &mySigAction, NULL);
	sigaction(SIGEMT, &mySigAction, NULL);
	sigaction(SIGFPE, &mySigAction, NULL);
	sigaction(SIGBUS, &mySigAction, NULL);
	sigaction(SIGSEGV, &mySigAction, NULL);
	sigaction(SIGSYS, &mySigAction, NULL);
}

int main(int argc, char * argv[])
{
	InstallUncaughtExceptionHandler();
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
