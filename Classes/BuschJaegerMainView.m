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
        [viewController viewWillAppear:animated];
    }
    [super pushViewController:viewController animated:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [self.topViewController viewDidAppear:animated];
        [oldTopViewController viewDidDisappear:animated];
    }
}

- (NSArray*)popToViewController:(UIViewController *)viewController animated:(BOOL)animated {
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        NSMutableArray *array = [NSMutableArray array];
        while([self.viewControllers count] > 1 && self.topViewController != viewController) {
            [array addObject:[self popViewControllerAnimated:animated]];
        }
        return array;
    }
    return [super popToViewController:viewController animated:animated];
}

- (UIViewController *)popViewControllerAnimated:(BOOL)animated {
    int count = [self.viewControllers count];
    if(count > 1) {
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [self.topViewController viewWillDisappear:animated];
            UIViewController *nextView = nil;
            nextView = [self.viewControllers objectAtIndex:count - 2];
            
            [nextView viewWillAppear:animated];
        }
        UIViewController * ret = [super popViewControllerAnimated:animated];
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [ret viewDidDisappear:animated];
            [self.topViewController viewDidAppear:animated];
        }
        return ret;
    }
    return nil;
}

@end

@implementation BuschJaegerMainView

@synthesize navigationController;
@synthesize callView;
@synthesize settingsView;
@synthesize manualSettingsView;
@synthesize welcomeView;
@synthesize historyView;
@synthesize historyDetailsView;

static BuschJaegerMainView* mainViewInstance=nil;



#pragma mark - Lifecycle Functions

- (void)initBuschJaegerMainView {
    assert (!mainViewInstance);
    mainViewInstance = self;
    loadCount = 0;
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
    [manualSettingsView release];
    [welcomeView release];
    [historyView release];
    [historyDetailsView release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    // Avoid IOS 4 bug
    if(loadCount++ > 0)
        return;
    
    [super viewDidLoad];
    
   // [self setWantsFullScreenLayout:TRUE];
    
    UIView *view = navigationController.view;
   // [view setFrame:[self.view bounds]];
    [self.view addSubview:view];
    [navigationController pushViewController:welcomeView animated:FALSE];
}

- (void)viewDidUnload {
    [super viewDidUnload];
    
    // Avoid IOS 4 bug
    loadCount--;
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

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return [navigationController.topViewController shouldAutorotateToInterfaceOrientation:interfaceOrientation];
}

- (NSUInteger)supportedInterfaceOrientations {
    return [navigationController.topViewController supportedInterfaceOrientations];
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
    if (![[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
        || [UIApplication sharedApplication].applicationState ==  UIApplicationStateActive) {
        [[LinphoneManager instance] setSpeakerEnabled:TRUE];
        AudioServicesPlaySystemSound([LinphoneManager instance].sounds.call);
    }
}

- (void)displayMessage:(id)message {
    if (![[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
		|| [UIApplication sharedApplication].applicationState ==  UIApplicationStateActive) {
        UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Dring !",nil)
                                                        message: [NSString stringWithFormat:NSLocalizedString(@"%@ ring!",nil), [LinphoneManager instance].configuration.levelPushButton.name]
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
