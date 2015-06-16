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

#include "linphone/linphonecore.h"
#include "UIPauseButton.h"
#import "UITransparentTVCell.h"

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

- (id)init:(LinphoneCall*) call minimized:(BOOL)minimized;

@property (nonatomic, strong) UIImage *image;
@property (nonatomic, strong) NSString *address;

@end

@interface UICallCell : UITransparentTVCell {
}

@property (nonatomic, strong) UICallCellData *data;

@property (nonatomic, strong) IBOutlet UIImageView* headerBackgroundImage;
@property (nonatomic, strong) IBOutlet UIImageView* headerBackgroundHighlightImage;

@property (nonatomic, strong) IBOutlet UILabel* addressLabel;
@property (nonatomic, strong) IBOutlet UILabel* stateLabel;
@property (nonatomic, strong) IBOutlet UIImageView* stateImage;
@property (nonatomic, strong) IBOutlet UIImageView* avatarImage;
@property (nonatomic, strong) IBOutlet UIButton *removeButton;
@property (nonatomic, strong) IBOutlet UIPauseButton *pauseButton;

@property (nonatomic, strong) IBOutlet UIView* headerView;
@property (nonatomic, strong) IBOutlet UIView* avatarView;

@property (nonatomic, strong) IBOutlet UIView* audioStatsView;

@property (nonatomic, strong) IBOutlet UILabel* audioCodecLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioCodecHeaderLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioUploadBandwidthLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioUploadBandwidthHeaderLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioDownloadBandwidthLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioDownloadBandwidthHeaderLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioIceConnectivityLabel;
@property (nonatomic, strong) IBOutlet UILabel* audioIceConnectivityHeaderLabel;

@property (nonatomic, strong) IBOutlet UIView* videoStatsView;

@property (nonatomic, strong) IBOutlet UILabel* videoCodecLabel;
@property (nonatomic, strong) IBOutlet UILabel* videoCodecHeaderLabel;

@property(strong, nonatomic) IBOutlet UILabel *videoSentSizeFPSHeaderLabel;
@property(strong, nonatomic) IBOutlet UILabel *videoSentSizeFPSLabel;
@property(strong, nonatomic) IBOutlet UILabel *videoRecvSizeFPSHeaderLabel;
@property(strong, nonatomic) IBOutlet UILabel *videoRecvSizeFPSLabel;

@property (nonatomic, strong) IBOutlet UILabel* videoUploadBandwidthLabel;
@property (nonatomic, strong) IBOutlet UILabel* videoUploadBandwidthHeaderLabel;
@property (nonatomic, strong) IBOutlet UILabel* videoDownloadBandwidthLabel;
@property (nonatomic, strong) IBOutlet UILabel* videoDownloadBandwidthHeaderLabel;
@property (nonatomic, strong) IBOutlet UILabel* videoIceConnectivityLabel;
@property (nonatomic, strong) IBOutlet UILabel* videoIceConnectivityHeaderLabel;

@property (nonatomic, strong) IBOutlet UIView* otherView;

@property (nonatomic, strong) IBOutlet UISwipeGestureRecognizer *detailsLeftSwipeGestureRecognizer;
@property (nonatomic, strong) IBOutlet UISwipeGestureRecognizer *detailsRightSwipeGestureRecognizer;

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
