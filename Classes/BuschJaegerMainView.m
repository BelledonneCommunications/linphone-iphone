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
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>

@implementation BuschJaegerMainView

@synthesize videoView;
@synthesize imageView;
@synthesize startCall;
@synthesize takeCall;
@synthesize decline;
@synthesize endOrRejectCall;
@synthesize mute;
@synthesize lights;
@synthesize openDoor;


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

#pragma mark - View lifecycle

-(void) createGradientForButton:(UIButton*) button withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor {
    CAGradientLayer* gradient = [CAGradientLayer layer];
    gradient.frame = button.bounds;
    gradient.colors = [NSArray arrayWithObjects:topColor.CGColor, bottomColor.CGColor, nil];
    [button.layer insertSublayer:gradient below:button.imageView.layer];   
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [openDoor initWithNumber:'1'];
    [lights initWithNumber:'2'];
    [mute initWithOnImage:[UIImage imageNamed:@"bj_mute_on.png"] offImage:[UIImage imageNamed:@"bj_mute_off.png"] debugName:"MUTE_BTN"];
    
    /* init gradients */
    {
        UIColor* col1 = [UIColor colorWithRed:32.0/255 green:45.0/255 blue:62.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:18.0/255 green:26.0/255 blue:41.0/255 alpha:1.0];
    
        [self createGradientForButton:startCall withTopColor:col1 bottomColor:col2];
        [self createGradientForButton:openDoor withTopColor:col1 bottomColor:col2];
        [self createGradientForButton:lights withTopColor:col1 bottomColor:col2];
        [self createGradientForButton:mute withTopColor:col1 bottomColor:col2];
    }
    {
        UIColor* col1 = [UIColor colorWithRed:153.0/255 green:48.0/255 blue:48.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:66.0/255 green:15.0/255 blue:15.0/255 alpha:1.0];
        
        [self createGradientForButton:endOrRejectCall withTopColor:col1 bottomColor:col2];
        [self createGradientForButton:decline withTopColor:col1 bottomColor:col2];
    }
    {
        UIColor* col1 = [UIColor colorWithRed:91.0/255 green:161.0/255 blue:89.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:25.0/255 green:54.0/255 blue:24.0/255 alpha:1.0];
        
        [self createGradientForButton:takeCall withTopColor:col1 bottomColor:col2];
    }
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
    
    [LinphoneManager set:startCall hidden:NO withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:takeCall hidden:YES withName:"TAKE_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:NO withName:"MUTE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:decline hidden:YES withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:endOrRejectCall hidden:YES withName:"END_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:YES withName:"VIDEO_VIEW" andReason:__FUNCTION__];
    
    
    if (!chatRoom) {
        NSString* s = [NSString stringWithFormat:@"sip:100000001@%@", [[NSUserDefaults standardUserDefaults] stringForKey:@"adapter_ip_preference"]];
        const char* adapter = [s cStringUsingEncoding:[NSString defaultCStringEncoding]];
        chatRoom = linphone_core_create_chat_room([LinphoneManager getLc], adapter);
        
        lights->chatRoom = chatRoom;
        openDoor->chatRoom = chatRoom;
    }
}

- (void) activateVideoView:(BOOL)value{
    if (value){
        linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)videoView);
    }else{
        linphone_core_set_native_video_window_id([LinphoneManager getLc],0);	
        linphone_core_set_native_preview_window_id([LinphoneManager getLc],0);
    }

}

- (void) viewDidDisappear:(BOOL)animated{
    
}

- (void) displayCall:(LinphoneCall *)call InProgressFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    /* nothing */
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

- (void) displayDialerFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    [LinphoneManager set:startCall hidden:NO withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:takeCall hidden:YES withName:"TAKE_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:NO withName:"MUTE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:decline hidden:YES withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:endOrRejectCall hidden:YES withName:"END_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:YES withName:"VIDEO_VIEW" andReason:__FUNCTION__];

    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

- (void) displayInCall:(LinphoneCall *)call FromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    [LinphoneManager set:startCall hidden:YES withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:takeCall hidden:YES withName:"TAKE_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:NO withName:"MUTE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:decline hidden:YES withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:endOrRejectCall hidden:NO withName:"END_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:NO withName:"VIDEO_VIEW" andReason:__FUNCTION__];
}

- (void) displayIncomingCall:(LinphoneCall *)call NotificationFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    [LinphoneManager set:startCall hidden:YES withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:takeCall hidden:NO withName:"TAKE_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:YES withName:"MUTE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:decline hidden:NO withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:endOrRejectCall hidden:YES withName:"END_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:NO withName:"VIDEO_VIEW" andReason:__FUNCTION__];
    
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
        && [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
        // Create a new notification
        UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
        if (notif)
        {
            notif.repeatInterval = 0;
            notif.alertBody = NSLocalizedString(@" Ding Dong !",nil);
            notif.alertAction = @"See the answer";
            notif.soundName = @BJ_RING_FILE ".wav";
            NSData *callData = [NSData dataWithBytes:&call length:sizeof(call)];
            notif.userInfo = [NSDictionary dictionaryWithObject:callData forKey:@"call"];
            
            [[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
        }
    }else{
        NSBundle* myBundle = [NSBundle mainBundle];
        
        NSString* path = [myBundle pathForResource:@BJ_RING_FILE ofType:@"wav"];
        if (path) {
            const char* soundfile = [path cStringUsingEncoding:[NSString defaultCStringEncoding]];
            ms_message("Using '%s' as ring file", soundfile);
            SystemSoundID sid;
            NSURL *pathURL = [NSURL fileURLWithPath : path];
            NSError *setCategoryError = nil;
            [[AVAudioSession sharedInstance]
             setCategory: AVAudioSessionCategoryAmbient
             error: &setCategoryError];
            //redirect audio to speaker
            UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
            AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
                                     , sizeof (audioRouteOverride)
                                     , &audioRouteOverride);

            AudioServicesCreateSystemSoundID((CFURLRef) pathURL, &sid);
            AudioServicesPlaySystemSound(sid);
        }

    }

    linphone_call_enable_camera(call, FALSE);
}

- (void) displayVideoCall:(LinphoneCall *)call FromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    [LinphoneManager set:startCall hidden:YES withName:"START_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:takeCall hidden:YES withName:"TAKE_CALL_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:mute hidden:NO withName:"MUTE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:decline hidden:YES withName:"DECLINE_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:endOrRejectCall hidden:NO withName:"END_BTN" andReason:__FUNCTION__];
    [LinphoneManager set:videoView hidden:NO withName:"VIDEO_VIEW" andReason:__FUNCTION__];
        
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

- (IBAction)takeCall:(id)sender {
    const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);	
    
    while(calls) {
        LinphoneCall* c = (LinphoneCall*) calls->data;
        if (linphone_call_get_state(c) == LinphoneCallIncoming || linphone_call_get_state(c) == LinphoneCallIncomingEarlyMedia) {
            linphone_core_accept_call([LinphoneManager getLc], c);
            return;
        }
        calls = calls->next;
    }
}

- (IBAction)startCall:(id)sender {
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
