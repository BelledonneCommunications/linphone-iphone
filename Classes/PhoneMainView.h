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
#import "CallDelegate.h"
#import "UICompositeViewController.h"
#import "UIModalViewController.h"
#import "AbstractCall.h"

typedef enum _PhoneView {
    PhoneView_Wizard,
    PhoneView_FirstLogin,
    PhoneView_Dialer,
    PhoneView_History,
    PhoneView_HistoryDetails,
    PhoneView_Settings,
    PhoneView_Chat,
    PhoneView_ChatRoom,
    PhoneView_Contacts,
    PhoneView_ContactDetails,
    PhoneView_InCall,
    PhoneView_IncomingCall,
    PhoneView_END
} PhoneView;

@interface PhoneMainView : UIViewController<CallActionSheetDelegate,UIModalViewDelegate> {
    @private
    UICompositeViewController *mainViewController;
    
    NSMutableDictionary *viewDescriptions;

    UIActionSheet *incomingCallActionSheet;
    UIActionSheet *batteryActionSheet;
    
    int loadCount;
    
    PhoneView currentView;
    NSMutableArray* viewStack;
}

@property (nonatomic, retain) IBOutlet UICompositeViewController *mainViewController;

- (void)changeView:(PhoneView)view;
- (void)changeView:(PhoneView)view push:(BOOL)push;
- (void)changeView:(PhoneView)view calls:(NSArray *)calls;
- (void)changeView:(PhoneView)view calls:(NSArray *)calls push:(BOOL)push;
- (void)popView;
- (void)popView:(NSArray *)calls;
- (void)showTabBar:(BOOL)show;
- (void)fullScreen:(BOOL)enabled;
- (PhoneView)currentView;

+ (PhoneMainView*) instance;

@end
