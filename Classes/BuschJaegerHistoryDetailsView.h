/* BuschJaegerHistoryDetailsView.h
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

#import <UIKit/UIKit.h>
#import "History.h"
#import "UIRemoteImageView.h"
#import "BuschJaegerConfiguration.h"

@interface BuschJaegerHistoryDetailsView : UIViewController<UITableViewDataSource, UITableViewDelegate, BuschJaegerConfigurationDelegate> {
@private
    int currentIndex;
    NSDateFormatter *dateFormatter;
}

@property (nonatomic, retain) IBOutlet UIView *backButton;
@property (nonatomic, retain) History *history;
@property (nonatomic, retain) IBOutlet UITableViewController *tableController;
@property (nonatomic, retain) IBOutlet UILabel *stationLabel;
@property (nonatomic, retain) IBOutlet UILabel *dateLabel;

@property (nonatomic, retain) IBOutlet UIView *fullscreenView;
@property (nonatomic, retain) IBOutlet UIButton *saveButton;
@property (nonatomic, retain) IBOutlet UIRemoteImageView *imageView;
@property (nonatomic, retain) IBOutlet UISwipeGestureRecognizer *detailsLeftSwipeGestureRecognizer;
@property (nonatomic, retain) IBOutlet UISwipeGestureRecognizer *detailsRightSwipeGestureRecognizer;
@property (nonatomic, retain) IBOutlet UITapGestureRecognizer *detailsTapGestureRecognizer;

- (IBAction)onBackClick:(id)sender;
- (IBAction)onDeleteClick:(id)sender;
- (IBAction)hideImage:(id)sender;
- (IBAction)saveImage:(id)sender;

@end
