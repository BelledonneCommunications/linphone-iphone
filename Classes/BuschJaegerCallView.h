/* BuschJaegerCallView.h
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

#import <UIKit/UIKit.h>

#import "linphonecore.h"
#import "UILinphone.h"
#import "UIDigitButton.h"
#import "UIHangUpButton.h"
#import "LinphoneManager.h"
#import "UIToggleButton.h"
#import "VideoZoomHandler.h"

@interface BuschJaegerCallView : UIViewController {
@private
    LinphoneChatRoom *chatRoom;
    LinphoneCall *currentCall;
    VideoZoomHandler* videoZoomHandler;
}

@property (nonatomic, retain) IBOutlet UIView* incomingView;
@property (nonatomic, retain) IBOutlet UILabel* contactLabel;
@property (nonatomic, retain) IBOutlet UIView* videoView;
@property (nonatomic, retain) IBOutlet UIButton* takeCallButton;
@property (nonatomic, retain) IBOutlet UIHangUpButton* declineButton;
@property (nonatomic, retain) IBOutlet UIHangUpButton* endOrRejectCallButton;
@property (nonatomic, retain) IBOutlet UIToggleButton* microButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* lightsButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* openDoorButton;
@property (nonatomic, retain) IBOutlet UIButton* snapshotButton;

- (IBAction)takeCall:(id)sender;
- (IBAction)onSnapshotClick:(id)sender;

@end
