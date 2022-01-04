/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "TPMultiLayoutViewController.h"
#include "linphone/linphonecore.h"
#import "UIRoundedImageView.h"
#import "UIDigitButton.h"


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
@property (weak, nonatomic) IBOutlet UIButton *declineButton;

@property (weak, nonatomic) IBOutlet UIToggleButton *numpadButton;
@property (strong, nonatomic) IBOutlet UIView *numpadView;
@property(nonatomic, strong) IBOutlet UIDigitButton *oneButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *twoButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *threeButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *fourButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *fiveButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *sixButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *sevenButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *eightButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *nineButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *starButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *zeroButton;
@property(nonatomic, strong) IBOutlet UIDigitButton *hashButton;
@property (weak, nonatomic) IBOutlet UIButton *declineButton_earlyMedia;

- (IBAction)onDeclineClick:(id)sender;

@end
