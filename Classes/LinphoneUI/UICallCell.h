/* UICallCell.h
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */  

#import <UIKit/UIKit.h>

#include "linphonecore.h"
#include "UIPauseButton.h"

typedef enum _UICallCellOtherView {
    UICallCellOtherView_Avatar = 0,
    UICallCellOtherView_AudioStats,
    UICallCellOtherView_VideoStats,
    UICallCellOtherView_MAX
} UICallCellOtherView;

@interface UICallCellData : NSObject {
    @public
    bool minimize;
    UICallCellOtherView view;
    LinphoneCall *call;
}   

- (id)init:(LinphoneCall*) call;

@property (nonatomic, retain) UIImage *image;
@property (nonatomic, retain) NSString *address;

@end

@interface UICallCell : UITableViewCell {
}

@property (nonatomic, retain) UICallCellData *data;

@property (nonatomic, retain) IBOutlet UIImageView* headerBackgroundImage;
@property (nonatomic, retain) IBOutlet UIImageView* headerBackgroundHighlightImage;

@property (nonatomic, retain) IBOutlet UILabel* addressLabel;
@property (nonatomic, retain) IBOutlet UILabel* stateLabel;
@property (nonatomic, retain) IBOutlet UIImageView* stateImage;
@property (nonatomic, retain) IBOutlet UIImageView* avatarImage;
@property (nonatomic, retain) IBOutlet UIButton *removeButton;
@property (nonatomic, retain) IBOutlet UIPauseButton *pauseButton;

@property (nonatomic, retain) IBOutlet UIView* headerView;
@property (nonatomic, retain) IBOutlet UIView* avatarView;

@property (nonatomic, retain) IBOutlet UIView* audioStatsView;

@property (nonatomic, retain) IBOutlet UILabel* audioCodecLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioCodecHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioUploadBandwidthLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioUploadBandwidthHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioDownloadBandwidthLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioDownloadBandwidthHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioIceConnectivityLabel;
@property (nonatomic, retain) IBOutlet UILabel* audioIceConnectivityHeaderLabel;

@property (nonatomic, retain) IBOutlet UIView* videoStatsView;

@property (nonatomic, retain) IBOutlet UILabel* videoCodecLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoCodecHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoUploadBandwidthLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoUploadBandwidthHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoDownloadBandwidthLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoDownloadBandwidthHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoIceConnectivityLabel;
@property (nonatomic, retain) IBOutlet UILabel* videoIceConnectivityHeaderLabel;

@property (nonatomic, retain) IBOutlet UIView* otherView;

@property (nonatomic, retain) IBOutlet UISwipeGestureRecognizer *detailsLeftSwipeGestureRecognizer;
@property (nonatomic, retain) IBOutlet UISwipeGestureRecognizer *detailsRightSwipeGestureRecognizer;

@property (assign) BOOL firstCell;
@property (assign) BOOL conferenceCell;
@property (nonatomic, assign) BOOL currentCall;

- (void)update;

- (id)initWithIdentifier:(NSString*)identifier;

- (IBAction)doHeaderClick:(id)sender;
- (IBAction)doRemoveClick:(id)sender;
- (IBAction)doDetailsSwipe:(UISwipeGestureRecognizer *)sender;
    
+ (int)getMaximizedHeight;
+ (int)getMinimizedHeight;

@end
