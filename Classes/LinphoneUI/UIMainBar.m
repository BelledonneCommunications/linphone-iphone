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
@synthesize chatButton;
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
    [chatButton release];
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
    
    // Update current view
    [self updateView:[[PhoneMainView instance] currentView]];
    if([LinphoneManager isLcReady]) {
        [self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc])];
    } else {
        [self updateMissedCall:0];
    }
    [self updateUnreadMessage:[ChatModel unreadMessages]];
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


#pragma mark - Event Functions

- (void)callUpdate: (NSNotification*) notif {  
    //LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    //LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc])];
}

- (void)changeViewEvent: (NSNotification*) notif {  
    NSNumber *viewNumber = [notif.userInfo objectForKey: @"view"];
    if(viewNumber != nil)
        [self updateView:[viewNumber intValue]];
}

- (void)textReceived: (NSNotification*) notif {  
    [self updateUnreadMessage:[ChatModel unreadMessages]];
}


#pragma mark - 

- (void)updateUnreadMessage:(int)unreadMessage {
    if (unreadMessage > 0) {
        if([chatNotificationView isHidden]) {
            chatNotificationView.transform = CGAffineTransformIdentity;
            [self startBounceAnimation:@"Bounce" target:chatNotificationView];
            [chatNotificationView setHidden:FALSE];
        }
        [chatNotificationLabel setText:[NSString stringWithFormat:@"%i", unreadMessage]];
    } else {
        if(![chatNotificationView isHidden]) {
            [self stopBounceAnimation:@"Bounce" target:chatNotificationView];
            CGAffineTransform startCGA = [chatNotificationView transform];
            [UIView animateWithDuration:0.4 
                                  delay:0 
                                options:UIViewAnimationOptionCurveEaseOut | UIViewAnimationOptionAllowUserInteraction
                             animations:^{
                                 chatNotificationView.transform = CGAffineTransformConcat(startCGA, CGAffineTransformMakeScale(0.01f, 0.01f));
                             }
                             completion:^(BOOL finished){
                                 [chatNotificationView setHidden:TRUE];
                             }
             ];
        }
    }
}

- (void)updateMissedCall:(int)missedCall {
    if (missedCall > 0) {
        if([historyNotificationView isHidden]) {
            historyNotificationView.transform = CGAffineTransformIdentity;
            [self startBounceAnimation:@"Bounce" target:historyNotificationView];
            [historyNotificationView setHidden:FALSE];
        }
        [historyNotificationLabel setText:[NSString stringWithFormat:@"%i", missedCall]];
    } else {
        if(![historyNotificationView isHidden]) {
            [self stopBounceAnimation:@"Bounce" target:historyNotificationView];
            CGAffineTransform startCGA = [historyNotificationView transform];
            [UIView animateWithDuration:0.4 
                                  delay:0 
                                options:UIViewAnimationOptionCurveEaseOut | UIViewAnimationOptionAllowUserInteraction
                             animations:^{
                                 historyNotificationView.transform = CGAffineTransformConcat(startCGA, CGAffineTransformMakeScale(0.01f, 0.01f));
                             }
                             completion:^(BOOL finished){
                                 [historyNotificationView setHidden:TRUE];
                             }
             ];
        }
    }
}

- (void)startBounceAnimation:(NSString *)animationID  target:(UIView *)target { 
    [target setTransform:CGAffineTransformMakeTranslation(0, -4)];
    [UIView animateWithDuration: 0.3
                          delay: 0
                        options: UIViewAnimationOptionRepeat | 
     UIViewAnimationOptionAutoreverse | 
     UIViewAnimationOptionAllowUserInteraction | 
     UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [target setTransform:CGAffineTransformMakeTranslation(0, 4)];
                     }
                     completion:^(BOOL finished){
                     }];
    
}

- (void)stopBounceAnimation:(NSString *)animationID target:(UIView *)target {
    [target.layer removeAnimationForKey:animationID];
}
         
- (void)updateView:(PhoneView) view {
    // Reset missed call
    if(view == PhoneView_History) {
        linphone_core_reset_missed_calls_count([LinphoneManager getLc]);
        [self updateMissedCall:0];
    }
    
    // Update buttons
    if(view == PhoneView_History || view == PhoneView_HistoryDetails) {
        historyButton.selected = TRUE;
    } else {
        historyButton.selected = FALSE;
    }
    if(view == PhoneView_Contacts || view == PhoneView_ContactDetails) {
        contactsButton.selected = TRUE;
    } else {
        contactsButton.selected = FALSE;
    }
    if(view == PhoneView_Dialer) {
        dialerButton.selected = TRUE;
    } else {
        dialerButton.selected = FALSE;
    }
    if(view == PhoneView_Settings) {
        settingsButton.selected = TRUE;
    } else {
        settingsButton.selected = FALSE;
    }
    if(view == PhoneView_Chat || view == PhoneView_ChatRoom) {
        chatButton.selected = TRUE;
    } else {
        chatButton.selected = FALSE;
    }
}


#pragma mark - Action Functions

- (IBAction)onHistoryClick: (id) sender {
    [[PhoneMainView instance] changeView:PhoneView_History];
}

- (IBAction)onContactsClick: (id) event {
    [[PhoneMainView instance] changeView:PhoneView_Contacts];
}

- (IBAction)onDialerClick: (id) event {
    [[PhoneMainView instance] changeView:PhoneView_Dialer];
}

- (IBAction)onSettingsClick: (id) event {
    [[PhoneMainView instance] changeView:PhoneView_Settings];
}

- (IBAction)onChatClick: (id) event {
    [[PhoneMainView instance] changeView:PhoneView_Chat];
}


@end
