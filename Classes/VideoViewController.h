/* VideoViewController.h
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */     
#import <UIKit/UIKit.h>
#import "UILinphone.h"

@interface VideoViewController : UIViewController {
	UIView* mPortrait;
	UIView* mDisplay;
	UIView* mPreview;
	UIMuteButton* mMute;
	UIHangUpButton* mHangUp;
	UICamSwitch* mCamSwitch;
	
	UIView* mLandscapeRight;
	UIView* mDisplayLandRight;
	UIView* mPreviewLandRight;
	UIMuteButton* mMuteLandRight;
	UIHangUpButton* mHangUpLandRight;
	UICamSwitch* mCamSwitchLandRight;
	
	UIView* mLandscapeLeft;
	UIView* mDisplayLandLeft;
	UIView* mPreviewLandLeft;
	UIMuteButton* mMuteLandLeft;
	UIHangUpButton* mHangUpLandLeft;
	UICamSwitch* mCamSwitchLandLeft;
	BOOL isFirst;
	int maxCall;

}

@property (nonatomic, retain) IBOutlet UIView* mPortrait;
@property (nonatomic, retain) IBOutlet UIView* mDisplay;
@property (nonatomic, retain) IBOutlet UIView* mPreview;
@property (nonatomic, retain) IBOutlet UIMuteButton* mMute;
@property (nonatomic, retain) IBOutlet UIHangUpButton* mHangUp;
@property (nonatomic, retain) IBOutlet UICamSwitch* mCamSwitch;
@property (nonatomic, retain) IBOutlet UIImageView *mCallQuality;

@property (nonatomic, retain) IBOutlet UIView* mLandscapeRight;
@property (nonatomic, retain) IBOutlet UIView* mDisplayLandRight;
@property (nonatomic, retain) IBOutlet UIView* mPreviewLandRight;
@property (nonatomic, retain) IBOutlet UIMuteButton* mMuteLandRight;
@property (nonatomic, retain) IBOutlet UIHangUpButton* mHangUpLandRight;
@property (nonatomic, retain) IBOutlet UICamSwitch* mCamSwitchLandRight;
@property (nonatomic, retain) IBOutlet UIImageView *mCallQualityLandRight;

@property (nonatomic, retain) IBOutlet UIView* mLandscapeLeft;
@property (nonatomic, retain) IBOutlet UIView* mDisplayLandLeft;
@property (nonatomic, retain) IBOutlet UIView* mPreviewLandLeft;
@property (nonatomic, retain) IBOutlet UIMuteButton* mMuteLandLeft;
@property (nonatomic, retain) IBOutlet UIHangUpButton* mHangUpLandLeft;
@property (nonatomic, retain) IBOutlet UICamSwitch* mCamSwitchLandLeft;
@property (retain, nonatomic) IBOutlet UIImageView *mCallQualityLandLeft;

- (void) updateCallQualityIndicator;

- (void) waitBeforeUpdatingCallQualityIndicator;

@end