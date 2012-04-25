/* IncallViewController.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
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
#import "linphonecore.h"
#import "PhoneViewController.h"
#import "ConferenceCallDetailView.h"
#import <AddressBookUI/ABPeoplePickerNavigationController.h>
#include "UILinphone.h"
#import "UIToggleVideoButton.h"
#import "VideoZoomHandler.h"
@class VideoViewController;

@interface IncallViewController : UIViewController <ABPeoplePickerNavigationControllerDelegate,LinphoneUICallDelegate, UITableViewDelegate, UITableViewDataSource, UIActionSheetCustomDelegate> {
	
	
	UIView* controlSubView, *hangUpView;
	
	UIButton* endCtrl;
	UIButton* dialer;
	UIMuteButton* mute;
    UIButton* pause;
	UISpeakerButton* speaker;
	UIButton* contacts;
	UIToggleVideoButton* addVideo;
    UITableView* callTableView;
    UIButton* addCall, *mergeCalls;
    UIButton* transfer;

	
	//key pad

	UIView* padSubView;
	UIDigitButton* one;
	UIDigitButton* two;
	UIDigitButton* three;
	UIDigitButton* four;
	UIDigitButton* five;
	UIDigitButton* six;
	UIDigitButton* seven;
	UIDigitButton* eight;
	UIDigitButton* nine;
	UIDigitButton* star;
	UIDigitButton* zero;
	UIDigitButton* hash;
	UIButton* close;
    
    UIView* videoGroup;
    UIView* videoView;
    UIView* videoPreview;
    UIImageView* videoCallQuality;
    UICamSwitch* videoCameraSwitch;
    UIActivityIndicatorView* videoUpdateIndicator;
    UIActivityIndicatorView* videoWaitingForFirstImage;
    
    bool dismissed;
    
    NSTimer *durationRefreasher;
    NSTimer * glowingTimer;
    
    float glow;
    NSIndexPath* activePath;
	
	ABPeoplePickerNavigationController* myPeoplePickerController;
    
    UITableViewCell* activeCallCell;
    
	VideoViewController* mVideoViewController;
    ConferenceCallDetailView* conferenceDetail;
    BOOL mVideoShown;
	BOOL mVideoIsPending;
	BOOL mIncallViewIsReady;
    
    UIImage* verified, *unverified;
	UIImage* stat_sys_signal_0, *stat_sys_signal_1, *stat_sys_signal_2, *stat_sys_signal_3, *stat_sys_signal_4;
    UIActionSheet* visibleActionSheet;

    NSTimer* hideControlsTimer;
    VideoZoomHandler* videoZoomHandler;
}

-(void)displayStatus:(NSString*) message;

- (IBAction)doAction:(id)sender;

+(LinphoneCall*) retrieveCallAtIndex: (NSInteger) index inConference:(bool) conf;
+ (void) updateCellImageView:(UIImageView*)imageView Label:(UILabel*)label DetailLabel:(UILabel*)detailLabel AndAccessoryView:(UIView*)accessoryView withCall:(LinphoneCall*) call;
+(void) updateIndicator:(UIImageView*) indicator withCallQuality:(float) quality;

@property (nonatomic, retain) IBOutlet UIView* controlSubView;
@property (nonatomic, retain) IBOutlet UIView* padSubView;
@property (nonatomic, retain) IBOutlet UIView* hangUpView;
@property (nonatomic, retain) IBOutlet UIViewController* conferenceDetail;

@property (nonatomic, retain) IBOutlet UIButton* endCtrl;
@property (nonatomic, retain) IBOutlet UIButton* dialer;
@property (nonatomic, retain) IBOutlet UIButton* mute;
@property (nonatomic, retain) IBOutlet UIButton* pause;
@property (nonatomic, retain) IBOutlet UIButton* speaker;
@property (nonatomic, retain) IBOutlet UIButton* contacts;
@property (nonatomic, retain) IBOutlet UIToggleVideoButton* addVideo;
@property (nonatomic, retain) IBOutlet UITableView* callTableView;
@property (nonatomic, retain) IBOutlet UIButton* addCall;
@property (nonatomic, retain) IBOutlet UIButton* mergeCalls;
@property (nonatomic, retain) IBOutlet UIButton* transfer;

@property (nonatomic, retain) IBOutlet UIButton* one;
@property (nonatomic, retain) IBOutlet UIButton* two;
@property (nonatomic, retain) IBOutlet UIButton* three;
@property (nonatomic, retain) IBOutlet UIButton* four;
@property (nonatomic, retain) IBOutlet UIButton* five;
@property (nonatomic, retain) IBOutlet UIButton* six;
@property (nonatomic, retain) IBOutlet UIButton* seven;
@property (nonatomic, retain) IBOutlet UIButton* eight;
@property (nonatomic, retain) IBOutlet UIButton* nine;
@property (nonatomic, retain) IBOutlet UIButton* star;
@property (nonatomic, retain) IBOutlet UIButton* zero;
@property (nonatomic, retain) IBOutlet UIButton* hash;
@property (nonatomic, retain) IBOutlet UIButton* close;
@property (nonatomic, retain) IBOutlet VideoViewController* videoViewController;

@property (nonatomic, retain) IBOutlet UIView* videoGroup;
@property (nonatomic, retain) IBOutlet UIView* videoView;
@property (nonatomic, retain) IBOutlet UIView* videoPreview;
@property (nonatomic, retain) IBOutlet UIImageView* videoCallQuality;
@property (nonatomic, retain) IBOutlet UICamSwitch* videoCameraSwitch;
@property (nonatomic, retain) IBOutlet UIActivityIndicatorView* videoUpdateIndicator;
@property (nonatomic, retain) IBOutlet UIActivityIndicatorView* videoWaitingForFirstImage;

@end
