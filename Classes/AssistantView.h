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
#import <XMLRPCConnectionDelegate.h>
#import "UICompositeView.h"
#import "TPKeyboardAvoidingScrollView.h"

@interface AssistantView : UIViewController <UITextFieldDelegate, UICompositeViewDelegate> {
  @private
	LinphoneAccountCreator *account_creator;
	UIView *currentView;
	UIView *nextView;
	NSMutableArray *historyViews;
	LinphoneProxyConfig *new_config;
	int number_of_configs_before;
}

@property(nonatomic, strong) IBOutlet TPKeyboardAvoidingScrollView *contentView;
@property(nonatomic, strong) IBOutlet UIView *waitView;
@property(nonatomic, strong) IBOutlet UIButton *backButton;

@property(nonatomic, strong) IBOutlet UIView *welcomeView;
@property(nonatomic, strong) IBOutlet UIView *createAccountView;
@property(nonatomic, strong) IBOutlet UIView *createAccountActivationView;
@property(nonatomic, strong) IBOutlet UIView *linphoneLoginView;
@property(nonatomic, strong) IBOutlet UIView *loginView;
@property(nonatomic, strong) IBOutlet UIView *remoteProvisioningLoginView;
@property(strong, nonatomic) IBOutlet UIView *remoteProvisioningView;

@property(nonatomic, strong) IBOutlet UIImageView *welcomeLogoImage;
@property(nonatomic, strong) IBOutlet UIButton *gotoCreateAccountButton;
@property(nonatomic, strong) IBOutlet UIButton *gotoLinphoneLoginButton;
@property(nonatomic, strong) IBOutlet UIButton *gotoLoginButton;
@property(nonatomic, strong) IBOutlet UIButton *gotoRemoteProvisioningButton;

+ (NSString *)errorForStatus:(LinphoneAccountCreatorStatus)status;

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

- (IBAction)onTransportChange:(id)sender;

@end
