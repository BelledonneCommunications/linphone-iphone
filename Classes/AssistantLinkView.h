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

#import "PhoneMainView.h"
#import <UIKit/UIKit.h>
#import "TPKeyboardAvoidingScrollView.h"

@interface AssistantLinkView : UIViewController <UITextFieldDelegate, UICompositeViewDelegate>
@property(weak, nonatomic) IBOutlet TPKeyboardAvoidingScrollView *linkAccountView;
@property(weak, nonatomic) IBOutlet UIView *activateSMSView;

@property(weak, nonatomic) IBOutlet UIButton *countryButton;
@property(weak, nonatomic) IBOutlet UITextField *countryCodeField;
@property(weak, nonatomic) IBOutlet UITextField *activationCodeField;
@property (weak, nonatomic) IBOutlet UIRoundBorderedButton *maybeLaterButton;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *linkAccountButton;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *checkValidationButton;
@property(weak, nonatomic) IBOutlet UIView *waitView;
@property(weak, nonatomic) IBOutlet UITextField *phoneField;
@property (weak, nonatomic) IBOutlet UILabel *linkSMSText;
@property BOOL firstTime;

- (IBAction)onLinkAccount:(id)sender;
- (IBAction)onCheckValidationButton:(id)sender;
- (IBAction)onCountryClick:(id)sender;
- (IBAction)onDialerClick:(id)sender;
- (IBAction)onPhoneNumberDisclosureClick:(id)sender;
- (IBAction)onMaybeLater:(id)sender;

@end
