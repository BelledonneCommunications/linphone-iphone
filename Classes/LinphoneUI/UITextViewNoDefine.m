//
//  UITextViewNoDefine.m
//  linphone
//
//  Created by guillaume on 05/03/2014.
//
//

#import "UITextViewNoDefine.h"

@implementation UITextViewNoDefine

@synthesize allowSelectAll;

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
	// disable "define" option, since it messes with the keyboard
	if ([[NSStringFromSelector(action) lowercaseString] rangeOfString:@"define"].location != NSNotFound) {
		return NO;
	} else if (action == @selector(selectAll:) && allowSelectAll) {
		return YES;
	} else {
		return [super canPerformAction:action withSender:sender];
	}
}

@end
