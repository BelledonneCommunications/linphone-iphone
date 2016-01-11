//
//  UIIconButton.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 22/07/15.
//
//

#import "UIIconButton.h"

#import "Utils.h"

@implementation UIIconButton

INIT_WITH_COMMON_CF {
	[super setImage:[self imageForState:UIControlStateNormal]
		   forState:(UIControlStateHighlighted | UIControlStateSelected)];
	[super setImage:[self imageForState:UIControlStateDisabled]
		   forState:(UIControlStateDisabled | UIControlStateSelected)];

	[self setBackgroundImage:[self backgroundImageForState:UIControlStateHighlighted]
					forState:(UIControlStateHighlighted | UIControlStateSelected)];
	[self setBackgroundImage:[self backgroundImageForState:UIControlStateDisabled]
					forState:(UIControlStateDisabled | UIControlStateSelected)];
	[LinphoneUtils buttonFixStates:self];
	[self.titleLabel setAdjustsFontSizeToFitWidth:TRUE];

	return self;
}

- (void)setImage:(UIImage *)image forState:(UIControlState)state {
	[super setImage:image forState:state];
	[self commonInit];
}
@end
