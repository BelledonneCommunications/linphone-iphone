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

- (id)fixBackgroundImageForState {
	[self setBackgroundImage:[self backgroundImageForState:UIControlStateHighlighted]
					forState:(UIControlStateHighlighted | UIControlStateSelected)];
	[self setBackgroundImage:[self backgroundImageForState:UIControlStateDisabled]
					forState:(UIControlStateDisabled | UIControlStateSelected)];
	[LinphoneUtils buttonFixStates:self];
	[self.titleLabel setAdjustsFontSizeToFitWidth:TRUE];

	return self;
}

- (id)init {
	return [[super init] fixBackgroundImageForState];
}

- (id)initWithCoder:(NSCoder *)aDecoder {
	return [[super initWithCoder:aDecoder] fixBackgroundImageForState];
}

- (id)initWithFrame:(CGRect)frame {
	return [[super initWithFrame:frame] fixBackgroundImageForState];
}

@end
