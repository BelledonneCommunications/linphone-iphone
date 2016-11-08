/* SettingsViewController.h
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
#import "IASKAppSettingsViewController.h"
#import "LinphoneCoreSettingsStore.h"

@interface SettingsView
	: UIViewController <IASKSettingsDelegate, UICompositeViewDelegate, MFMailComposeViewControllerDelegate> {
  @private
	LinphoneCoreSettingsStore *settingsStore;
}

@property(nonatomic, strong) IBOutlet UINavigationController *navigationController;
@property(nonatomic, strong) IBOutlet IASKAppSettingsViewController *settingsController;
@property(weak, nonatomic) IBOutlet UIView *subView;
@property(weak, nonatomic) IBOutlet UIButton *backButton;
@property(weak, nonatomic) IBOutlet UILabel *titleLabel;
@property(nonatomic) NSString* tmpPwd;

- (IBAction)onDialerBackClick:(id)sender;
- (IBAction)onBackClick:(id)sender;

@end
