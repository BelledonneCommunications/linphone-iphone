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
    CGFloat height =self.frame.size.height;
    CGFloat witdh = self.frame.size.width;
    CGFloat roundRadius = height > witdh ? witdh / 2 : height / 2;

    [imageLayer setCornerRadius:roundRadius];
    [imageLayer setBorderWidth:0];
    [imageLayer setMasksToBounds:YES];
}


@end
