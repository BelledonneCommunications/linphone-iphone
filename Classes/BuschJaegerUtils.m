/* BuschJaegerUtils.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "BuschJaegerUtils.h"

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>

@implementation BuschJaegerUtils

+ (void)createGradientForView:(UIView*)view withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor {
    [BuschJaegerUtils createGradientForView:view withTopColor:topColor bottomColor:bottomColor cornerRadius:0];
}

+ (void)createGradientForButton:(UIButton*)button withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor {
    [BuschJaegerUtils createGradientForButton:button withTopColor:topColor bottomColor:bottomColor cornerRadius:0];
}

+ (void)createGradientForView:(UIView*)view withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor cornerRadius:(int)corner{
    // Remove previous
    for(CALayer *sublayer in view.layer.sublayers) {
        if(sublayer.name != nil && [sublayer.name compare:@"BuschJaegerLayer" options:0] == NSOrderedSame) {
            [sublayer removeFromSuperlayer];
            break;
        }
    };
    CAGradientLayer* gradient = [CAGradientLayer layer];
    gradient.needsDisplayOnBoundsChange = TRUE;
    gradient.name = @"BuschJaegerLayer";
    gradient.frame = view.bounds;
    gradient.cornerRadius = corner;
    gradient.colors = [NSArray arrayWithObjects:(id)topColor.CGColor, (id)bottomColor.CGColor, nil];
    [view.layer insertSublayer:gradient atIndex:0];
}

+ (void)createGradientForButton:(UIButton*)button withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor cornerRadius:(int)corner{
    // Remove previous
    for(CALayer *sublayer in button.layer.sublayers) {
        if(sublayer.name != nil && [sublayer.name compare:@"BuschJaegerLayer" options:0] == NSOrderedSame) {
            [sublayer removeFromSuperlayer];
            break;
        }
    };
    CAGradientLayer* gradient = [CAGradientLayer layer];
    gradient.needsDisplayOnBoundsChange = TRUE;
    gradient.name = @"BuschJaegerLayer";
    gradient.frame = button.bounds;
    gradient.cornerRadius = corner;
    gradient.colors = [NSArray arrayWithObjects:(id)topColor.CGColor, (id)bottomColor.CGColor, nil];
    [button.layer insertSublayer:gradient below:button.imageView.layer];
}

+ (void)resizeGradientLayer:(CALayer*)layer {
    if(layer.name != nil && [layer.name compare:@"BuschJaegerLayer" options:0] == NSOrderedSame) {
        if(layer.delegate == nil) {
            [layer setFrame:layer.superlayer.bounds];
        } else if([layer.delegate isKindOfClass:[UIView class]]) {
            [layer setFrame:((UIView *)layer.delegate).frame];
        }
    }
    if([layer respondsToSelector:@selector(sublayers)]) {
        for(CALayer *sublayer in layer.sublayers) {
            [BuschJaegerUtils resizeGradientLayer:sublayer];
        };
    }
}

+ (void)resizeGradient:(UIView*)view {
    [BuschJaegerUtils resizeGradientLayer:view.layer];
}

@end
