/* OutgoingCallViewController.h
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

#import "UICompositeView.h"
#import "TPMultiLayoutViewController.h"
#include "linphone/linphonecore.h"
#import "UIRoundedImageView.h"

@interface CallOutgoingView : TPMultiLayoutViewController <UICompositeViewDelegate> {
}
@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UISpeakerButton *speakerButton;
@property(weak, nonatomic) IBOutlet UILabel *addressLabel;
@property(weak, nonatomic) IBOutlet UIToggleButton *routesButton;
@property(weak, nonatomic) IBOutlet UIView *routesView;
@property(weak, nonatomic) IBOutlet UIBluetoothButton *routesBluetoothButton;
@property(weak, nonatomic) IBOutlet UIButton *routesEarpieceButton;
@property(weak, nonatomic) IBOutlet UISpeakerButton *routesSpeakerButton;
@property(weak, nonatomic) IBOutlet UIMutedMicroButton *microButton;

- (IBAction)onDeclineClick:(id)sender;

@end
