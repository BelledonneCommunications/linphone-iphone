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

#import "UIHistoryCell.h"
#import "LinphoneManager.h"

@implementation UIHistoryCell

@synthesize history;
@synthesize iconImage;
@synthesize stationLabel;
@synthesize dateLabel;


#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIHistoryCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
        
        dateFormatter = [[NSDateFormatter alloc] init];
        [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
        [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
        NSLocale *locale = [NSLocale currentLocale];
        [dateFormatter setLocale:locale];
    }
    return self;
}

- (void)dealloc {
    [dateFormatter release];
    
    [history release];
    [iconImage release];
    [stationLabel release];
    [dateLabel release];
    [super dealloc];
}


#pragma mark - Property Functions

- (void)setHistory:(History *)ahistory {
    if(ahistory == history) {
        return;
    }
    [history release];
    history = [ahistory retain];
    [self update];
}


#pragma mark - 

- (void)update {
    NSString *stationName = @"Unknown";
    NSSet *set = [[[LinphoneManager instance].configuration outdoorStations] filteredSetUsingPredicate:[NSPredicate predicateWithFormat:@"ID == %i", history.stationID]];
    if([set count] == 1) {
        OutdoorStation *station = [[set allObjects] objectAtIndex:0];
        stationName = station.name;
    }
    // Station
    [stationLabel setText:stationName];
    
    // Date
    [dateLabel setText:[dateFormatter stringFromDate:history.date]];
    
    if([history.images count] > 0) {
        NSString *image = [history.images objectAtIndex:0];
        [iconImage setImage:nil];
        [iconImage loadImage:[[LinphoneManager instance].configuration getImageUrl:image]];
    }
}


#pragma mark - Action Functions

- (IBAction)onDeleteClick: (id) event {
    if(history != NULL) {
        UIView *view = [self superview];
        // Find TableViewCell
        if(view != nil && ![view isKindOfClass:[UITableView class]]) view = [view superview];
        if(view != nil) {
            UITableView *tableView = (UITableView*) view;
            NSIndexPath *indexPath = [tableView indexPathForCell:self];
            [[tableView dataSource] tableView:tableView commitEditingStyle:UITableViewCellEditingStyleDelete forRowAtIndexPath:indexPath];
        }
    }
}


@end
