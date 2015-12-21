//
//  UITextField+DoneButton.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 14/10/14.
//
//

#import "UITextField+DoneButton.h"

#import "LinphoneManager.h"

@implementation UITextField (DoneButton)

- (void)addDoneButton {
	// actually on iPad there is a done button
	if (!IPAD) {
		UIToolbar *numberToolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, 320, 50)];
		numberToolbar.items = [NSArray
			arrayWithObjects:[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Cancel", nil)
															  style:UIBarButtonItemStyleBordered
															 target:self
															 action:@selector(cancelNumberPad)],
							 [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
																		   target:nil
																		   action:nil],
							 [[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Done", nil)
															  style:UIBarButtonItemStyleDone
															 target:self
															 action:@selector(doneWithNumberPad)],
							 nil];
		[numberToolbar sizeToFit];

		self.inputAccessoryView = numberToolbar;
	}
}

- (void)cancelNumberPad {
	[self resignFirstResponder];
	self.text = @"";
}

- (void)doneWithNumberPad {
	[self resignFirstResponder];
}
@end
