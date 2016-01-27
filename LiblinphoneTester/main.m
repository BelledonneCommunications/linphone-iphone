//
//  main.m
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

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
	@autoreleasepool {
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
