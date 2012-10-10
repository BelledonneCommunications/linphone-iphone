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

@implementation UINavigationControllerEx

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated {
    [viewController view];
    UIViewController *oldTopViewController = self.topViewController;
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [oldTopViewController viewWillDisappear:animated];
    }
    [viewController viewWillAppear:animated];
    [super pushViewController:viewController animated:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [self.topViewController viewDidAppear:animated];
        [oldTopViewController viewDidDisappear:animated];
    }
}

- (UIViewController *)popViewControllerAnimated:(BOOL)animated {
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [self.topViewController viewWillDisappear:animated];
        UIViewController *nextView = nil;
        int count = [self.viewControllers count];
        if(count > 1) {
            nextView = [self.viewControllers objectAtIndex:count - 2];
        }
        [nextView viewWillAppear:animated];
    }
    UIViewController * ret = [super popViewControllerAnimated:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [ret viewDidDisappear:animated];
        [self.topViewController viewDidAppear:animated];
    }
    return ret;
}

@end

@implementation BuschJaegerMainView

@synthesize navigationController;
@synthesize callView;
@synthesize settingsView;
@synthesize welcomeView;
@synthesize historyView;
@synthesize historyDetailsView;

static BuschJaegerMainView* mainViewInstance=nil;



#pragma mark - Lifecycle Functions

- (void)initBuschJaegerMainView {
    assert (!mainViewInstance);
    mainViewInstance = self;
}

- (id)init {
    self = [super init];
    if (self) {
		[self initBuschJaegerMainView];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self initBuschJaegerMainView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initBuschJaegerMainView];
	}
    return self;
}

- (void)dealloc {
    [navigationController release];
    [callView release];
    [settingsView release];
    [welcomeView release];
    [historyView release];
    [historyDetailsView release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self setWantsFullScreenLayout:TRUE];
    
    UIView *view = navigationController.view;
    [view setFrame:[self.view bounds]];
    [self.view addSubview:view];
    [navigationController pushViewController:welcomeView animated:FALSE];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(callUpdateEvent:)
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textReceivedEvent:)
                                                 name:kLinphoneTextReceived
                                               object:nil];
}

- (void)vieWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneCallUpdate
                                                  object:nil];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneTextReceived
                                                  object:nil];
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskPortraitUpsideDown | UIInterfaceOrientationMaskPortrait;
}


#pragma mark - Event Functions

- (void)callUpdateEvent: (NSNotification*) notif {
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:call state:state animated:TRUE];
}


- (void)textReceivedEvent: (NSNotification*) notif {
    [self displayMessage:notif];
}


#pragma mark -

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
    // Fake call update
    if(call == NULL) {
        return;
    }
    
    switch (state) {
		case LinphoneCallIncomingReceived:
        {
            [self displayIncomingCall:call];
        }
        case LinphoneCallOutgoingInit:
        {
            linphone_call_enable_camera(call, FALSE);
        }
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
        case LinphoneCallUpdating:
        {
            [navigationController popToViewController:welcomeView animated:FALSE];
            [navigationController pushViewController:callView animated:FALSE]; // No animation... Come back when Apple have learned how to create a good framework
            break;
        }
        case LinphoneCallError:
		case LinphoneCallEnd:
        {
            if ((linphone_core_get_calls([LinphoneManager getLc]) == NULL)) {
                [navigationController popToViewController:welcomeView animated:FALSE]; // No animation... Come back when Apple have learned how to create a good framework
            }
			break;
        }
        default:
            break;
	}
}

- (void)displayIncomingCall:(LinphoneCall *)call {
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
        && [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
        
        NSString *ringtone = [NSString stringWithFormat:@"%@_loop.wav", [[NSUserDefaults standardUserDefaults] stringForKey:@"ringtone_preference"], nil];
        
        NSString *contactName = NSLocalizedString(@"Unknown", nil);
        
        // Extract caller address
        const LinphoneAddress* addr = linphone_call_get_remote_address(call);
        if(addr) {
            char *address = linphone_address_as_string_uri_only(addr);
            if(address != NULL) {
                contactName = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:address]];
                ms_free(address);
            }
        }
        
        // Find caller in outdoor stations
        NSSet *outstations = [[LinphoneManager instance] configuration].outdoorStations;
        for(OutdoorStation *os in outstations) {
            if([[FastAddressBook normalizeSipURI:os.address] isEqualToString:contactName]) {
                contactName = os.name;
                break;
            }
        }
        
        NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"%@ ring!",nil), contactName];
        
        // Create a new notification
        UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
        if (notif) {
            notif.repeatInterval = 0;
            notif.alertBody = msg;
            notif.alertAction = NSLocalizedString(@"Answer", nil);
            notif.soundName = ringtone;
            NSData *callData = [NSData dataWithBytes:&call length:sizeof(call)];
            notif.userInfo = [NSDictionary dictionaryWithObject:callData forKey:@"call"];
            
            [[UIApplication sharedApplication] presentLocalNotificationNow:notif];
        }
    }else{
        [[LinphoneManager instance] setSpeakerEnabled:TRUE];
        AudioServicesPlaySystemSound([LinphoneManager instance].sounds.call);
    }
}

- (void)displayMessage:(id)message {
    NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"%@ ring!",nil), [LinphoneManager instance].configuration.levelPushButton.name];
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
		&& [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
        
        NSString *ringtone = [NSString stringWithFormat:@"%@_loop.wav", [[NSUserDefaults standardUserDefaults] stringForKey:@"level_ringtone_preference"], nil];
        
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif) {
			notif.repeatInterval = 0;
			notif.alertBody = msg;
			notif.alertAction = NSLocalizedString(@"Show", nil);
			notif.soundName = ringtone;
			
			[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
		}
	} else {
        UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Dring !",nil)
                                                        message:msg
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
        [error show];
        [error release];
        [[LinphoneManager instance] setSpeakerEnabled:TRUE];
        AudioServicesPlayAlertSound([LinphoneManager instance].sounds.level);
    }
}

+ (BuschJaegerMainView *) instance {
    return mainViewInstance;
}


@end
