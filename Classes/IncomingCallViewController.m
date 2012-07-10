/* IncomingCallViewController.m
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

#import "IncomingCallViewController.h"
#import "LinphoneManager.h"

@implementation IncomingCallViewController

@synthesize addressLabel;
@synthesize avatarImage;
@synthesize call;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"IncomingCallViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdateEvent:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [self callUpdate:call state:linphone_call_get_state(call)];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
}



#pragma mark - Event Functions

- (void)callUpdateEvent:(NSNotification*)notif {  
    LinphoneCall *acall = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState astate = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:acall state:astate];
}


#pragma mark - 

- (void)callUpdate:(LinphoneCall *)acall state:(LinphoneCallState)astate {  
    if(call == acall && (astate == LinphoneCallEnd || astate == LinphoneCallError)) {
        [self dismiss: [NSNumber numberWithInt: IncomingCall_Aborted]];
    }
}


#pragma mark - Property Functions

- (void)setCall:(LinphoneCall*)acall {
    [self view]; //Force view load
    
    call = acall;
    const char* userNameChars=linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* userName = userNameChars?[[[NSString alloc] initWithUTF8String:userNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    const char* displayNameChars =  linphone_address_get_display_name(linphone_call_get_remote_address(call));        
	NSString* displayName = [displayNameChars?[[NSString alloc] initWithUTF8String:displayNameChars]:@"" autorelease];
    
    [addressLabel setText:([displayName length]>0)?displayName:userName];
}

- (LinphoneCall*) getCall {
    return call;
}


#pragma mark - Action Functions

- (IBAction)onAcceptClick:(id)event {
    linphone_core_accept_call([LinphoneManager getLc], call);
    [self dismiss: [NSNumber numberWithInt:IncomingCall_Accepted]];
}

- (IBAction)onDeclineClick:(id)event {
    linphone_core_terminate_call([LinphoneManager getLc], call);
    [self dismiss: [NSNumber numberWithInt:IncomingCall_Decline]];
}

@end
