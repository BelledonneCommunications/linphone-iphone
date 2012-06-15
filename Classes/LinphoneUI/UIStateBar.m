/* UIStateBar.m
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

#import "UIStateBar.h"
#import "LinphoneManager.h"

#include "linphonecore.h"

@implementation UIStateBar

@synthesize registrationStateImage;
@synthesize registrationStateLabel;
@synthesize callQualityImage;

NSTimer *callQualityTimer;

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(registrationUpdate:) name:@"LinphoneRegistrationUpdate" object:nil];
    
    // Set callQualityTimer
    [callQualityImage setHidden: true];
	callQualityTimer = [NSTimer scheduledTimerWithTimeInterval:1 
													 target:self 
												   selector:@selector(callQualityUpdate) 
												   userInfo:nil 
													repeats:YES];
    
    // Update to default state
    [self proxyConfigUpdate: NULL];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)registrationUpdate: (NSNotification*) notif {  
    LinphoneProxyConfig* config = NULL;
    linphone_core_get_default_proxy([LinphoneManager getLc], &config);
    [self proxyConfigUpdate:config];
}

- (void)callQualityUpdate { 
    UIImage *image = nil;
    if([LinphoneManager isLcReady]) {
        LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
        if(call != NULL) {
            float quality = linphone_call_get_average_quality(call);
            if(quality < 1) {
                image = [UIImage imageNamed:@"quality-call-0.png"];
            } else if (quality < 2) {
                image = [UIImage imageNamed:@"quality-call-1.png"];
            } else if (quality < 3) {
                image = [UIImage imageNamed:@"quality-call-2.png"];
            } else {
                image = [UIImage imageNamed:@"quality-call-3.png"];
            }
        }
    }
    if(image != nil) {
        [callQualityImage setHidden: false];
        [callQualityImage setImage: image];
    } else {
        [callQualityImage setHidden: true];
    }
}

- (void)proxyConfigUpdate: (LinphoneProxyConfig*) config {
    LinphoneRegistrationState state;
    NSString* message = nil;
    UIImage* image = nil;

    if (config == NULL) {
        state = LinphoneRegistrationNone;
        if(![LinphoneManager isLcReady] || linphone_core_is_network_reachabled([LinphoneManager getLc]))
            message = NSLocalizedString(@"No SIP account configured", nil);
        else
            message = NSLocalizedString(@"Network down", nil);
    } else {
        state = linphone_proxy_config_get_state(config);
    
        switch (state) {
            case LinphoneRegistrationOk: 
                message = @"Registered"; break;
            case LinphoneRegistrationNone: 
            case LinphoneRegistrationCleared:
                message = @"Not registered"; break;
            case LinphoneRegistrationFailed: 
                message = @"Registration failed"; break;
            case LinphoneRegistrationProgress: 
                message = @"Registration in progress"; break;
                //case LinphoneRegistrationCleared: m= @"No SIP account"; break;
            default: break;
        }
    }

    registrationStateLabel.hidden = NO;
    switch(state) {
        case LinphoneRegistrationCleared:
            /*            image.hidden = NO;
             [image setImage:[UIImage imageNamed:@"status_orange.png"]];
             [spinner stopAnimating];
             [label setText:message != nil ? message : NSLocalizedString(@"No SIP account defined", nil)];*/
            break;
        case LinphoneRegistrationFailed:
            registrationStateImage.hidden = NO;
            image = [UIImage imageNamed:@"status_error.png"];
            break;
        case LinphoneRegistrationNone:
            registrationStateImage.hidden = NO;
            image =[UIImage imageNamed:@"status_disconnected.png"];
            break;
        case LinphoneRegistrationProgress:
            registrationStateImage.hidden = NO;
            image = [UIImage imageNamed:@"status_inprogress.png"];
            break;
        case LinphoneRegistrationOk:
            registrationStateImage.hidden = NO;
            image = [UIImage imageNamed:@"status_connected.png"];
            break;
    }
    [registrationStateLabel setText:message];
    [registrationStateImage setImage:image];
}

- (void) viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [callQualityTimer invalidate];
}

- (void) dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [callQualityTimer invalidate];
    [callQualityTimer release];
    [super dealloc];
}

@end
