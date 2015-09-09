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

#import <MediaPlayer/MediaPlayer.h>

/* These imports are here so that we can import PhoneMainView.h without bothering to import all the rest of the view headers */
#import "AboutViewController.h"
#import "ChatRoomViewController.h"
#import "ChatViewController.h"
#import "ContactDetailsLabelViewController.h"
#import "ContactDetailsViewController.h"
#import "ContactsViewController.h"
#import "DialerViewController.h"
#import "HistoryDetailsViewController.h"
#import "HistoryViewController.h"
#import "ImageViewController.h"
#import "InCallViewController.h"
#import "IncomingCallViewController.h"
#import "OutgoingCallViewController.h"
#import "SettingsViewController.h"
#import "SideMenuViewController.h"
#import "AssistantViewController.h"

#import "DTAlertView.h"
#import "DTActionSheet.h"
#import "Utils.h"

@class PhoneMainView;

@interface RootViewManager : NSObject

@property (nonatomic, strong) PhoneMainView* portraitViewController;
@property (nonatomic, strong) PhoneMainView* rotatingViewController;
@property (nonatomic, strong) NSMutableArray* viewDescriptionStack;

+(RootViewManager*)instance;
+ (void)setupWithPortrait:(PhoneMainView*)portrait;
- (PhoneMainView*)currentView;

@end

@interface PhoneMainView : UIViewController<IncomingCallViewDelegate> {
    @private
    NSMutableArray *inhibitedEvents;
}

@property (nonatomic, strong) IBOutlet UIView *statusBarBG;
@property (nonatomic, strong) IBOutlet UICompositeViewController *mainViewController;

@property (nonatomic, strong) NSString* name;
@property (weak, readonly) UICompositeViewDescription *currentView;
@property (readonly, strong) MPVolumeView* volumeView;

- (UIViewController*)changeCurrentView:(UICompositeViewDescription *)currentView;
- (UIViewController*)changeCurrentView:(UICompositeViewDescription *)currentView push:(BOOL)push;
- (UIViewController *)changeCurrentView:(UICompositeViewDescription *)view push:(BOOL)push animated:(BOOL)animated;
- (UIViewController*)popCurrentView;
- (void)popToView:(UICompositeViewDescription *)currentView;
- (UICompositeViewDescription *)firstView;
- (void)showStateBar:(BOOL)show;
- (void)showTabBar:(BOOL)show;
- (void)fullScreen:(BOOL)enabled;
- (void)updateStatusBar:(UICompositeViewDescription*)to_view;
- (void)startUp;
- (void)displayIncomingCall:(LinphoneCall*) call;
- (void)setVolumeHidden:(BOOL)hidden;

- (void)addInhibitedEvent:(id)event;
- (BOOL)removeInhibitedEvent:(id)event;

- (void)updateApplicationBadgeNumber;
+ (PhoneMainView*) instance;

@end
