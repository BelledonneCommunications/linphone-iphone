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
        layer.cornerRadius = 8.0f;
        layer.masksToBounds = YES;
        layer.borderWidth = 1.0f;
        layer.borderColor = [UIColor colorWithWhite:0.5f alpha:0.2f].CGColor;
        
        CAGradientLayer *overlayButtonShineLayer;
        
        overlayButtonShineLayer = [CAGradientLayer layer];
        overlayButtonShineLayer.name = @"BuschJaegerLayer";
        overlayButtonShineLayer.frame = layer.bounds;
        overlayButtonShineLayer.colors = [NSArray arrayWithObjects:
                                          (id)[UIColor colorWithWhite:1.0f
                                                                alpha:1.0].CGColor,
                                          (id)[UIColor colorWithWhite:1.0f
                                                                alpha:1.0f].CGColor,
                                          (id)[UIColor colorWithRed:0.0f green:0.0f blue:0.5f
                                                                alpha:0.8f].CGColor,
                                          (id)[UIColor colorWithRed:0.0f green:0.0f blue:0.2f
                                                                alpha:0.8f].CGColor,
                                          nil];
        overlayButtonShineLayer.locations = [NSArray arrayWithObjects:
                                             [NSNumber numberWithFloat:0.0f],
                                             [NSNumber numberWithFloat:0.1f],
                                             [NSNumber numberWithFloat:0.101f],
                                             [NSNumber numberWithFloat:1.0f],
                                             nil];
        [layer addSublayer:overlayButtonShineLayer];
        
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
