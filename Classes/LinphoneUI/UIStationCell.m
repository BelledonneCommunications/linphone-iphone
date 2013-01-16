/* UIStationCell.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "UIStationCell.h"
#import "BuschJaegerUtils.h"
#import <QuartzCore/QuartzCore.h>

@implementation UIStationCell

@synthesize stationImage;
@synthesize stationLabel;
@synthesize station;
@synthesize cellBackgroundView;

#pragma mark - Lifecycle Functions

- (void)roundView:(UIView *)view onCorner:(UIRectCorner)rectCorner radius:(float)radius
{
    UIBezierPath *maskPath = [UIBezierPath bezierPathWithRoundedRect:view.bounds
                                                   byRoundingCorners:rectCorner
                                                         cornerRadii:CGSizeMake(radius, radius)];
    CAShapeLayer *maskLayer = [[CAShapeLayer alloc] init];
    maskLayer.frame = view.bounds;
    maskLayer.path = maskPath.CGPath;
    [view.layer setMask:maskLayer];
    [maskLayer release];
}

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIStationCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
        
        CALayer *layer = cellBackgroundView.layer;
        layer.name = @"BuschJaegerLayer";
        
        CAGradientLayer *overlayButtonShineLayer;
        
        [self roundView:cellBackgroundView onCorner:(UIRectCornerBottomLeft|UIRectCornerBottomRight) radius:7.0];
        
        overlayButtonShineLayer = [CAGradientLayer layer];
        overlayButtonShineLayer.name = @"BuschJaegerLayer";
        overlayButtonShineLayer.frame = layer.bounds;
        overlayButtonShineLayer.colors = [NSArray arrayWithObjects:
                                          (id)[UIColor colorWithWhite:1.0f
                                                                alpha:1.0].CGColor,
                                          (id)[UIColor colorWithWhite:1.0f
                                                                alpha:1.0f].CGColor,
                                          (id)[UIColor colorWithRed:0x2f/255.0f green:0x48/255.0f blue:0x63/255.0f
                                                                alpha:1].CGColor,
                                          (id)[UIColor colorWithRed:0x1c/255.0f green:0x27/255.0f blue:0x3b/255.0f
                                                              alpha:1].CGColor,
                                          nil];
        overlayButtonShineLayer.locations = [NSArray arrayWithObjects:
                                             [NSNumber numberWithFloat:0.0f],
                                             [NSNumber numberWithFloat:0.02f],
                                             [NSNumber numberWithFloat:0.021f],
                                             [NSNumber numberWithFloat:1.0f],
                                             nil];
        [layer addSublayer:overlayButtonShineLayer];
        
        UIView * shadow = [[UIView alloc] initWithFrame:cellBackgroundView.frame];
        shadow.userInteractionEnabled = NO; // Modify this if needed
        shadow.layer.shadowColor = [[UIColor blackColor] CGColor];
        shadow.layer.shadowOffset = CGSizeMake(2, 2);
        shadow.layer.shadowRadius = 5.0f;
        shadow.layer.masksToBounds = NO;
        shadow.clipsToBounds = NO;
        shadow.layer.shadowOpacity = 0.9f;
        [cellBackgroundView.superview insertSubview:shadow belowSubview:cellBackgroundView];
        [shadow addSubview:cellBackgroundView];
    }
    return self;
}

- (void)dealloc {
    [station release];
    [stationImage release];
    [stationLabel release];
    
    [cellBackgroundView release];
    [super dealloc];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [BuschJaegerUtils resizeGradient:self];
}

#pragma mark - Property Functions

- (void)setStation:(OutdoorStation *)astation {
    if(astation == station) {
        return;
    }
    [station release];
    station = [astation retain];
    [self update];
}


#pragma mark - 

- (void)update {
    [stationLabel setText:station.name];
}

@end
