/* UIMainBar.m
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

#import "UIMainBar.h"
#import "PhoneMainView.h"
#import "ChatModel.h"

@implementation UIMainBar

@synthesize historyButton;
@synthesize contactsButton;
@synthesize dialerButton;
@synthesize settingsButton;
/* MODIFICATION Remove chat
@synthesize chatButton;
 */
@synthesize moreButton;

@synthesize historyNotificationView;
@synthesize historyNotificationLabel;
@synthesize chatNotificationView;
@synthesize chatNotificationLabel;

#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"UIMainBar" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [historyButton release];
    [contactsButton release];
    [dialerButton release];
    [settingsButton release];
    [moreButton release];
/*
    [chatButton release];
 */
    [historyNotificationView release];
    [historyNotificationLabel release];
    [chatNotificationView release];
    [chatNotificationLabel release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(changeViewEvent:) 
                                                 name:@"LinphoneMainViewChange" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceived:) 
                                                 name:@"LinphoneTextReceived" 
                                               object:nil];
    [self update];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneMainViewChange" 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneCallUpdate" 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneTextReceived" 
                                                  object:nil];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(applicationWillEnterForeground:) 
                                                 name:UIApplicationWillEnterForegroundNotification 
                                               object:nil];
}

- (void)viewDidUnload {
    [super viewDidUnload];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:UIApplicationWillEnterForegroundNotification 
                                                  object:nil];
}


#pragma mark - Event Functions

- (void)applicationWillEnterForeground:(NSNotification*)notif { 
    // Force the animations 
    [[self.view layer] removeAllAnimations];
    [chatNotificationView setHidden:TRUE];
    [historyNotificationView setHidden:TRUE];
    [self update];
}

- (void)callUpdate:(NSNotification*)notif {  
    //LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    //LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc])];
}

- (void)changeViewEvent:(NSNotification*)notif {  
    //UICompositeViewDescription *view = [notif.userInfo objectForKey: @"view"];
    //if(view != nil)
    [self updateView:[[PhoneMainView instance] firstView]];
}

- (void)textReceived:(NSNotification*)notif {  
    [self updateUnreadMessage:[ChatModel unreadMessages]];
}


#pragma mark - 

- (void)update {
    [self updateView:[[PhoneMainView instance] firstView]];
    if([LinphoneManager isLcReady]) {
        [self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc])];
    } else {
        [self updateMissedCall:0];
    }
    [self updateUnreadMessage:[ChatModel unreadMessages]];
}

- (void)updateUnreadMessage:(int)unreadMessage{
    if (unreadMessage > 0) {
        if([chatNotificationView isHidden]) {
            [chatNotificationView setHidden:FALSE];
            [self appearAnimation:@"Appear" target:chatNotificationView completion:^(BOOL finished){
                [self startBounceAnimation:@"Bounce" target:chatNotificationView];
            }];
        }
        [chatNotificationLabel setText:[NSString stringWithFormat:@"%i", unreadMessage]];
    } else {
        if(![chatNotificationView isHidden]) {
            [self stopBounceAnimation:@"Bounce" target:chatNotificationView];
            [self disappearAnimation:@"Disappear" target:chatNotificationView completion:^(BOOL finished){
                [chatNotificationView setHidden:TRUE];
            }];
        }
    }
}

- (void)updateMissedCall:(int)missedCall{
    if (missedCall > 0) {
        if([historyNotificationView isHidden]) {
            [historyNotificationView setHidden:FALSE];
            [self appearAnimation:@"Appear" target:historyNotificationView completion:^(BOOL finished){
                [self startBounceAnimation:@"Bounce" target:historyNotificationView];
            }];
        }
        [historyNotificationLabel setText:[NSString stringWithFormat:@"%i", missedCall]];
    } else {
        if(![historyNotificationView isHidden]) {
            [self stopBounceAnimation:@"Bounce" target:historyNotificationView];
            [self disappearAnimation:@"Disappear" target:historyNotificationView completion:^(BOOL finished){
                                 [historyNotificationView setHidden:TRUE];
                             }
             ];
        }
    }
}

- (void)appearAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    target.transform = CGAffineTransformMakeScale(0.01f, 0.01f);
    [UIView animateWithDuration:0.4 
                          delay:0 
                        options:UIViewAnimationOptionCurveEaseOut | UIViewAnimationOptionAllowUserInteraction
                     animations:^{
                         target.transform = CGAffineTransformIdentity;
                     }
                     completion:completion];
}

- (void)disappearAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    CGAffineTransform startCGA = [target transform];
    [UIView animateWithDuration:0.4 
                          delay:0 
                        options:UIViewAnimationOptionCurveEaseOut | UIViewAnimationOptionAllowUserInteraction
                     animations:^{
                         target.transform = CGAffineTransformConcat(startCGA, CGAffineTransformMakeScale(0.01f, 0.01f));
                     }
                     completion:completion];
}

- (void)startBounceAnimation:(NSString *)animationID target:(UIView *)target { 
    CGAffineTransform startCGA = [target transform];
    [UIView animateWithDuration: 0.3
                          delay: 0
                        options: UIViewAnimationOptionRepeat | 
     UIViewAnimationOptionAutoreverse | 
     UIViewAnimationOptionAllowUserInteraction | 
     UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [target setTransform: CGAffineTransformConcat(startCGA, CGAffineTransformMakeTranslation(0, 8))];
                     }
                     completion:^(BOOL finished){
                     }];
    
}

- (void)stopBounceAnimation:(NSString *)animationID target:(UIView *)target {
    [target.layer removeAnimationForKey:animationID];
}
         
- (void)updateView:(UICompositeViewDescription*) view {
    // Reset missed call
    if([view equal:[HistoryViewController compositeViewDescription]]) {
        linphone_core_reset_missed_calls_count([LinphoneManager getLc]);
        [self updateMissedCall:0];
    }
    
    // Update buttons
    if([view equal:[HistoryViewController compositeViewDescription]]) {
        historyButton.selected = TRUE;
    } else {
        historyButton.selected = FALSE;
    }
    if([view equal:[ContactsViewController compositeViewDescription]]) {
        contactsButton.selected = TRUE;
    } else {
        contactsButton.selected = FALSE;
    }
    if([view equal:[DialerViewController compositeViewDescription]]) {
        dialerButton.selected = TRUE;
    } else {
        dialerButton.selected = FALSE;
    }
    if([view equal:[SettingsViewController compositeViewDescription]]) {
        settingsButton.selected = TRUE;
    } else {
        settingsButton.selected = FALSE;
    }
    
    /* MODIFICATION Remove chat
    if([view equal:[ChatViewController compositeViewDescription]]) {
        chatButton.selected = TRUE;
    } else {
        moreButton.selected = FALSE;
    }*/
}


#pragma mark - Action Functions

- (IBAction)onHistoryClick: (id) sender {
    [[PhoneMainView instance] changeCurrentView:[HistoryViewController compositeViewDescription]];
}

- (IBAction)onContactsClick: (id) event {
    [ContactSelection setSelectionMode:ContactSelectionModeNone];
    [ContactSelection setAddAddress:nil];
    [ContactSelection setSipFilter:FALSE];
    [[PhoneMainView instance] changeCurrentView:[ContactsViewController compositeViewDescription]];
}

- (IBAction)onDialerClick: (id) event {
    [[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
}

- (IBAction)onSettingsClick: (id) event {
    [[PhoneMainView instance] changeCurrentView:[SettingsViewController compositeViewDescription]];
}

/* MODIFICATION Remove chat
- (IBAction)onChatClick: (id) event {
    [[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
}
*/

- (IBAction)onMoreClick: (id) event {
    //[[PhoneMainView instance] changeView:PhoneView_Chat];
}

@end
