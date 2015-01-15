//
//  UIRoundedImageView.m
//  linphone
//
//  Created by guillaume on 13/05/2014.
//
//

#import "UIRoundedImageView.h"
#import <QuartzCore/QuartzCore.h>

@implementation UIRoundedImageView


- (id) init {
    self = [super init];
    if (self ){
        [self setRoundRadius:TRUE];
    }
    return self;
}

- (void) setImage:(UIImage *)image {
	[self setImage:image withRoundedRadius:TRUE];
}

- (void) setImage:(UIImage *)image withRoundedRadius:(BOOL)rounded {
    [super setImage:image];
    [self setRoundRadius:rounded];
}

- (void)setRoundRadius:(BOOL)radius {
    CALayer *imageLayer = self.layer;
	CGRect frame = imageLayer.frame;
    CGFloat height =self.frame.size.height;
    CGFloat width = self.frame.size.width;
    CGFloat roundRadius = height > width ? width / 2 : height / 2;

	if (height > width) {
		frame.origin.y = height / 2 - width / 2;
		frame.size.height = width;
	} else {
		frame.origin.x = width / 2 - height / 2;
		frame.size.width = height;
	}
	[imageLayer setFrame:frame];
    [imageLayer setCornerRadius:roundRadius];
    [imageLayer setBorderWidth:0];
    [imageLayer setMasksToBounds:YES];
}


@end
