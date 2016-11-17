/* AssistantViewController.h
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
#import "UICompositeView.h"
#import "TPKeyboardAvoidingScrollView.h"
#import "PhoneMainView.h"

@interface AssistantView : UIViewController <UITextFieldDelegate, UICompositeViewDelegate> {

  @private
	LinphoneAccountCreator *account_creator;
	UIView *currentView;
	UIView *nextView;
	NSMutableArray *historyViews;
	LinphoneProxyConfig *new_config;
	size_t number_of_configs_before;
	BOOL mustRestoreView;
	long phone_number_length;
}

@property(nonatomic) UICompositeViewDescription *outgoingView;
@property (weak, nonatomic) IBOutlet UILabel *subtileLabel_useLinphoneAccount;

@property(nonatomic, strong) IBOutlet TPKeyboardAvoidingScrollView *contentView;
@property(nonatomic, strong) IBOutlet UIView *waitView;
@property(nonatomic, strong) IBOutlet UIButton *backButton;
@property (weak, nonatomic) IBOutlet UIButton *infoLoginButton;

@property(nonatomic, strong) IBOutlet UIView *welcomeView;
@property(nonatomic, strong) IBOutlet UIView *createAccountView;
@property(nonatomic, strong) IBOutlet UIView *createAccountActivateEmailView;
@property(nonatomic, strong) IBOutlet UIView *linphoneLoginView;
@property(nonatomic, strong) IBOutlet UIView *loginView;
@property(nonatomic, strong) IBOutlet UIView *remoteProvisioningLoginView;
@property(strong, nonatomic) IBOutlet UIView *remoteProvisioningView;
@property (strong, nonatomic) IBOutlet UIView *createAccountActivateSMSView;

@property(nonatomic, strong) IBOutlet UIImageView *welcomeLogoImage;
@property(nonatomic, strong) IBOutlet UIButton *gotoCreateAccountButton;
@property(nonatomic, strong) IBOutlet UIButton *gotoLinphoneLoginButton;
@property(nonatomic, strong) IBOutlet UIButton *gotoLoginButton;
@property(nonatomic, strong) IBOutlet UIButton *gotoRemoteProvisioningButton;
@property (weak, nonatomic) IBOutlet UILabel *phoneLabel;
@property (weak, nonatomic) IBOutlet UILabel *phoneTitle;
@property (weak, nonatomic) IBOutlet UILabel *activationTitle;
@property (weak, nonatomic) IBOutlet UILabel *activationEmailText;
@property (weak, nonatomic) IBOutlet UILabel *activationSMSText;

@property (weak, nonatomic) IBOutlet UILabel *accountLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *createAccountNextButtonPositionConstraint;

+ (NSString *)errorForStatus:(LinphoneAccountCreatorStatus)status;
+ (NSString *)StringForXMLRPCError:(const char *)err;

- (void)reset;
- (void)fillDefaultValues;

- (IBAction)onBackClick:(id)sender;
- (IBAction)onDialerClick:(id)sender;

- (IBAction)onGotoCreateAccountClick:(id)sender;
- (IBAction)onGotoLinphoneLoginClick:(id)sender;
- (IBAction)onGotoLoginClick:(id)sender;
- (IBAction)onGotoRemoteProvisioningClick:(id)sender;

- (IBAction)onCreateAccountClick:(id)sender;
- (IBAction)onCreateAccountActivationClick:(id)sender;
- (IBAction)onLinphoneLoginClick:(id)sender;
- (IBAction)onLoginClick:(id)sender;
- (IBAction)onRemoteProvisioningLoginClick:(id)sender;
- (IBAction)onRemoteProvisioningDownloadClick:(id)sender;
- (IBAction)onCreateAccountCheckActivatedClick:(id)sender;
- (IBAction)onLinkAccountClick:(id)sender;

- (IBAction)onFormSwitchToggle:(id)sender;
- (IBAction)onCountryCodeClick:(id)sender;
- (IBAction)onCountryCodeFieldChange:(id)sender;
- (IBAction)onCountryCodeFieldEnd:(id)sender;
- (IBAction)onPhoneNumberDisclosureClick:(id)sender;
@end
