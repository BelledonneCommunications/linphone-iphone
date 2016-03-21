/* StatusBarViewController.h
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
#import "TPMultiLayoutViewController.h"
#import "UIConfirmationDialog.h"
#import "LinphoneManager.h"

@interface StatusBarView : UIViewController {
	UIConfirmationDialog *securityDialog;
}

@property(weak, nonatomic) IBOutlet UIButton *registrationState;
@property(nonatomic, strong) IBOutlet UIButton *callSecurityButton;
@property(weak, nonatomic) IBOutlet UIButton *voicemailButton;
@property(weak, nonatomic) IBOutlet UIButton *callQualityButton;

@property(weak, nonatomic) IBOutlet UIView *incallView;
@property(weak, nonatomic) IBOutlet UIView *outcallView;

- (IBAction)onSecurityClick:(id)sender;
- (IBAction)onSideMenuClick:(id)sender;
- (IBAction)onRegistrationStateClick:(id)sender;
+ (UIImage *)imageForState:(LinphoneRegistrationState)state;
@end
