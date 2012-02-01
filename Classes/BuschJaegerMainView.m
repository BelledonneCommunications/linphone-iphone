/* BuschJaegerMainView.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import "BuschJaegerMainView.h"
#include "linphonecore.h"

@implementation BuschJaegerMainView

@synthesize videoView;
@synthesize imageView;
@synthesize startCall;
@synthesize stopCall;
@synthesize declineCall;
@synthesize mute;
@synthesize lights;
@synthesize openDoor;


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [openDoor initWithNumber:'1'];
    [lights initWithOnImage:[UIImage imageNamed:@"icon5"] offImage:[UIImage imageNamed:@"icon6"] debugName:"LIGHT_BTN"];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void) viewDidAppear:(BOOL)animated {
    [[LinphoneManager instance] setRegistrationDelegate:self];
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

- (void) displayCall:(LinphoneCall *)call InProgressFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    /* nothing */
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

- (void) displayDialerFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    [LinphoneManager set:stopCall hidden:YES withName:"STOP_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:startCall hidden:NO withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:YES withName:"VIDEO_VIEW" andReason:__FUNCTION__];
    [LinphoneManager set:declineCall hidden:YES withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:NO withName:"MUTE_BTN" andReason:__FUNCTION__];
    
    // [LinphoneManager set:imageView hidden:NO withName:"IMAGE_VIEW" andReason:__FUNCTION__];
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

- (void) displayInCall:(LinphoneCall *)call FromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    
}

- (void) displayIncomingCall:(LinphoneCall *)call NotificationFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
        && [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
        // Create a new notification
        UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
        if (notif)
        {
            notif.repeatInterval = 0;
            notif.alertBody = NSLocalizedString(@" Ding Dong !",nil);
            notif.alertAction = @"See the answer";
            notif.soundName = @"oldphone-mono-30s.caf";
            NSData *callData = [NSData dataWithBytes:&call length:sizeof(call)];
            notif.userInfo = [NSDictionary dictionaryWithObject:callData forKey:@"call"];
            
            [[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
        }
    }
        
    [LinphoneManager set:stopCall hidden:YES withName:"STOP_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:startCall hidden:NO withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:NO withName:"VIDEO_VIEW" andReason:__FUNCTION__];
    [LinphoneManager set:declineCall hidden:NO withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:YES withName:"MUTE_BTN" andReason:__FUNCTION__];
    
    linphone_call_enable_camera(call, FALSE);
}

- (void) displayVideoCall:(LinphoneCall *)call FromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    [LinphoneManager set:stopCall hidden:NO withName:"STOP_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:startCall hidden:YES withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:NO withName:"VIDEO_VIEW" andReason:__FUNCTION__];
    [LinphoneManager set:declineCall hidden:YES withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:NO withName:"MUTE_BTN" andReason:__FUNCTION__];
}

- (void) displayStatus:(NSString *)message {
    
}

- (void) displayNotRegisteredFromUI:(UIViewController *)viewCtrl {
    
}

- (void) displayRegisteredFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName onDomain:(NSString *)domain {
    
}

- (void) displayRegisteringFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName onDomain:(NSString *)domain {
    
}

- (void) displayRegistrationFailedFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName onDomain:(NSString *)domain forReason:(NSString *)reason {
    
}

- (void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex withUserDatas:(void *)datas {
    
}

- (IBAction)acceptCall:(id)sender {
    const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);	
    
    while(calls) {
        LinphoneCall* c = (LinphoneCall*) calls->data;
        if (linphone_call_get_state(c) == LinphoneCallIncoming || linphone_call_get_state(c) == LinphoneCallIncomingEarlyMedia) {
            linphone_core_accept_call([LinphoneManager getLc], c);
            return;
        }
        calls = calls->next;
    }
    
    // no pending call, call adapter
    NSString* s = [NSString stringWithFormat:@"sip:100000001@%@", [[NSUserDefaults standardUserDefaults] stringForKey:@"adapter_ip_preference"]];
    const char* adapter = [s cStringUsingEncoding:[NSString defaultCStringEncoding]];
    ms_message("Calling ADAPTER '%s'", adapter);
    LinphoneCallParams* lcallParams = linphone_core_create_default_call_parameters([LinphoneManager getLc]);
    linphone_call_params_enable_video(lcallParams, true);
    LinphoneCall* lc = linphone_core_invite_with_params([LinphoneManager getLc], adapter,lcallParams);
    if (!lc) {
        ms_error("Failed to start a new call");
        return;
    }
    linphone_call_enable_camera(lc, false);  
    linphone_call_params_destroy(lcallParams);
}

@end
