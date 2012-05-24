/* linphoneAppDelegate.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
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
#import <AddressBookUI/ABPeoplePickerNavigationController.h>
#import "CoreTelephony/CTCallCenter.h"
#import "IASKAppSettingsViewController.h"

#define HISTORY_TAB_INDEX 0
#define DIALER_TAB_INDEX 1
#define CONTACTS_TAB_INDEX 2
#define SETTINGS_TAB_INDEX 3
#define MORE_TAB_INDEX 4

@class ContactPickerDelegate;
@class IncallViewController;
@class PhoneViewController;
@class CallHistoryTableViewController;


@interface linphoneAppDelegate : NSObject <UIApplicationDelegate,UIAlertViewDelegate> {
    UIWindow *window;
	IBOutlet UITabBarController*  myTabBarController;
	IBOutlet ABPeoplePickerNavigationController* myPeoplePickerController;
	IBOutlet PhoneViewController* myPhoneViewController;
    IBOutlet UINavigationController* moreNavigationController;
    IBOutlet IASKAppSettingsViewController* settingsController;
	CallHistoryTableViewController* myCallHistoryTableViewController;
	ContactPickerDelegate* myContactPickerDelegate;
    
    CTCallCenter* callCenter;
}

- (void) loadDefaultSettings:(NSDictionary *) appDefaults;
-(void) setupUI;
-(void) setupGSMInteraction;

-(void) startApplication;

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController*  myTabBarController;
@property (nonatomic, retain) ABPeoplePickerNavigationController* myPeoplePickerController;
@property (nonatomic, retain) IBOutlet UINavigationController* moreNavigationController;
@property (nonatomic, retain) IBOutlet PhoneViewController* myPhoneViewController;
@property (nonatomic, retain) IBOutlet IASKAppSettingsViewController* settingsController;

@end

