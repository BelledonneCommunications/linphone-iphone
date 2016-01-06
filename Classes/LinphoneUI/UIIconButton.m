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

	[super setImage:[self imageForState:UIControlStateSelected]
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

- (id)init {
	return [[super init] fixBackgroundImageForState];
}

- (id)initWithCoder:(NSCoder *)aDecoder {
	return [[super initWithCoder:aDecoder] fixBackgroundImageForState];
}

- (id)initWithFrame:(CGRect)frame {
	return [[super initWithFrame:frame] fixBackgroundImageForState];
}

- (void)setImage:(UIImage *)image forState:(UIControlState)state {
	[super setImage:image forState:state];
	[self fixBackgroundImageForState];
}
@end
