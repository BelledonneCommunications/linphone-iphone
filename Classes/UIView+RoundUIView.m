//
//  UIView+RoundUIView.m
//  linphone
//
//  Created by Jörg Platte on 16.01.13.
//
//

#import "UIView+RoundUIView.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@implementation UIView (RoundUIView)

- (void)makeRoundWithCorner:(UIRectCorner)rectCorner radius:(float)radius {
    UIBezierPath *maskPath = [UIBezierPath bezierPathWithRoundedRect:self.bounds
                                                   byRoundingCorners:rectCorner
                                                         cornerRadii:CGSizeMake(radius, radius)];
    CAShapeLayer *maskLayer = [[CAShapeLayer alloc] init];
    maskLayer.frame = self.bounds;
    maskLayer.path = maskPath.CGPath;
    [self.layer setMask:maskLayer];
    [maskLayer release];
}


@end
