/* UIHistoryCell.m
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

#import "UIHistoryDetailsCell.h"
#import "LinphoneManager.h"

@implementation UIHistoryDetailsCell

@synthesize image;
@synthesize imageView;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIHistoryDetailsCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            UIView *view = [[arrayOfViews objectAtIndex:0] retain];
            [view setFrame:[self bounds]];
            [self addSubview:view];
        }
    }
    return self;
}

- (void)dealloc {   
    [image release];
    [imageView release];
    [super dealloc];
}


#pragma mark - Property Functions

- (void)setCustomImage:(NSString *)aimage {
    if(aimage == image) {
        return;
    }
    [image release];
    image = [aimage retain];
    [self update];
}


#pragma mark - 

- (void)update {
    if(image) {
        [imageView setImage:nil];
        [imageView loadImage:[[LinphoneManager instance].configuration getImageUrl:image]];
    }
}

@end
