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
#import "UIAssistantTextField.h"

@interface FirstLoginView : TPMultiLayoutViewController <UITextFieldDelegate, UICompositeViewDelegate> {
	LinphoneAccountCreator *account_creator;
}

- (IBAction)onLoginClick:(id)sender;
- (IBAction)onSiteClick:(id)sender;

@property(nonatomic, strong) IBOutlet UIButton *loginButton;
@property(nonatomic, strong) IBOutlet UIButton *siteButton;
@property(nonatomic, strong) IBOutlet UIAssistantTextField *usernameField;
@property(nonatomic, strong) IBOutlet UIAssistantTextField *passwordField;
@property(nonatomic, strong) IBOutlet UIView *waitView;
@property(weak, nonatomic) IBOutlet UIAssistantTextField *domainField;

@end
