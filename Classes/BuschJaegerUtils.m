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
    CAGradientLayer* gradient = [CAGradientLayer layer];
    gradient.frame = view.bounds;
    gradient.colors = [NSArray arrayWithObjects:(id)topColor.CGColor, (id)bottomColor.CGColor, nil];
    [view.layer insertSublayer:gradient atIndex:0];
}

+ (void)createGradientForButton:(UIButton*)button withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor {
    CAGradientLayer* gradient = [CAGradientLayer layer];
    gradient.frame = button.bounds;
    gradient.colors = [NSArray arrayWithObjects:(id)topColor.CGColor, (id)bottomColor.CGColor, nil];
    [button.layer insertSublayer:gradient below:button.imageView.layer];
}

@end
