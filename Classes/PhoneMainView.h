/* PhoneMainView.h
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

#import "LinphoneManager.h"
#import "UICompositeViewController.h"

#import "FirstLoginViewController.h"
#import "IncomingCallViewController.h"
#import "ChatRoomViewController.h"
#import "ChatViewController.h"
#import "DialerViewController.h"
#import "ContactsViewController.h"
#import "ContactDetailsViewController.h"
#import "ContactDetailsLabelViewController.h"
#import "ContactDetailsImagePickerController.h"
#import "HistoryViewController.h"
#import "HistoryDetailsViewController.h"
#import "InCallViewController.h"
#import "SettingsViewController.h"
#import "FirstLoginViewController.h"
#import "WizardViewController.h"
#import "IncomingCallViewController.h"
#import "ConsoleViewController.h"
#import "ImageViewController.h"

@interface PhoneMainView : UIViewController<CallActionSheetDelegate, IncomingCallViewDelegate> {
    @private
    UIActionSheet *batteryActionSheet;
    int loadCount;
    NSMutableArray *viewStack;
    NSMutableArray *inhibitedEvents;
}

@property (nonatomic, retain) IBOutlet UICompositeViewController *mainViewController;
@property (readonly) UICompositeViewDescription *currentView;
- (UIViewController*)changeCurrentView:(UICompositeViewDescription *)currentView;
- (UIViewController*)changeCurrentView:(UICompositeViewDescription *)currentView push:(BOOL)push;
- (UIViewController*)popCurrentView;
- (void)popToView:(UICompositeViewDescription *)currentView;
- (UICompositeViewDescription *)firstView;
- (void)showStateBar:(BOOL)show;
- (void)showTabBar:(BOOL)show;
- (void)fullScreen:(BOOL)enabled;
- (void)startUp;

+ (void)setOrientation:(UIInterfaceOrientation)orientation animated:(BOOL)animated;

- (void)addInhibitedEvent:(id)event;
- (BOOL)removeInhibitedEvent:(id)event;

+ (PhoneMainView*) instance;

@end
