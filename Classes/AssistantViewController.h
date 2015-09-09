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
#import "UICompositeViewController.h"
#import "TPKeyboardAvoidingScrollView.h"

@interface AssistantViewController : TPMultiLayoutViewController <UITextFieldDelegate, UICompositeViewDelegate,
																  XMLRPCConnectionDelegate, UIAlertViewDelegate> {
  @private
	UITextField *activeTextField;
	UIView *currentView;
	UIView *nextView;
	NSMutableArray *historyViews;
}

@property(nonatomic, weak) IBOutlet TPKeyboardAvoidingScrollView *contentView;
@property(nonatomic, weak) IBOutlet UIView *waitView;
@property(nonatomic, weak) IBOutlet UIButton *backButton;

@property(nonatomic, weak) IBOutlet UIView *welcomeView;
@property(nonatomic, weak) IBOutlet UIView *createAccountView;
@property(nonatomic, weak) IBOutlet UIView *createAccountActivationView;
@property(nonatomic, weak) IBOutlet UIView *linphoneLoginView;
@property(nonatomic, weak) IBOutlet UIView *loginView;
@property(weak, nonatomic) IBOutlet UIView *remoteProvisionningView;

@property(nonatomic, weak) IBOutlet UIImageView *welcomeLogoImage;
@property(nonatomic, weak) IBOutlet UIButton *gotoCreateAccountButton;
@property(nonatomic, weak) IBOutlet UIButton *gotoLinphoneLoginButton;
@property(nonatomic, weak) IBOutlet UIButton *gotoLoginButton;
@property(weak, nonatomic) IBOutlet UIButton *gotoRemoteProvisionningButton;

- (void)reset;
- (void)fillDefaultValues;

- (IBAction)onBackClick:(id)sender;
- (IBAction)onDialerBackClick:(id)sender;

- (IBAction)onGotoCreateAccountClick:(id)sender;
- (IBAction)onGotoLinphoneLoginClick:(id)sender;
- (IBAction)onGotoLoginClick:(id)sender;
- (IBAction)onGotoRemoteProvisionningClick:(id)sender;

- (IBAction)onCreateAccountClick:(id)sender;
- (IBAction)onCreateAccountActivationClick:(id)sender;
- (IBAction)onLinphoneLoginClick:(id)sender;
- (IBAction)onLoginClick:(id)sender;
- (IBAction)onRemoteProvisionningClick:(id)sender;

@end
