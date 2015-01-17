/* PhoneMainView.m
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

#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioServices.h>

#import "LinphoneAppDelegate.h"
#import "PhoneMainView.h"
#import "Utils.h"
#import "DTActionSheet.h"


static RootViewManager* rootViewManagerInstance = nil;

@implementation RootViewManager {
    PhoneMainView* currentViewController;
}

+ (void)setupWithPortrait:(PhoneMainView*)portrait {
    assert(rootViewManagerInstance == nil);
    rootViewManagerInstance = [[RootViewManager alloc]initWithPortrait:portrait];
}

- (instancetype)initWithPortrait:(PhoneMainView*)portrait {
    self = [super init];
    if ( self ){
        self.portraitViewController = portrait;
        self.rotatingViewController = [[[PhoneMainView alloc] init] autorelease];

        self.portraitViewController.name = @"Portrait";
        self.rotatingViewController.name = @"Rotating";

        currentViewController = portrait;
        self.viewDescriptionStack = [NSMutableArray array];

    }
    return self;
}

+ (RootViewManager *)instance {
    if( !rootViewManagerInstance ){
        @throw [NSException exceptionWithName:@"RootViewManager" reason:@"nil instance" userInfo:nil];
    }
    return rootViewManagerInstance;
}

- (PhoneMainView*)currentView {
    return currentViewController;
}

- (PhoneMainView*)setViewControllerForDescription:(UICompositeViewDescription*)description {
    PhoneMainView* newMainView = description.landscapeMode ? self.rotatingViewController : self.portraitViewController;

    if( [LinphoneManager runningOnIpad] ) return currentViewController;

    if( newMainView != currentViewController )
    {
        PhoneMainView* previousMainView = currentViewController;
        UIInterfaceOrientation nextViewOrientation = newMainView.interfaceOrientation;
        UIInterfaceOrientation previousOrientation = currentViewController.interfaceOrientation;

        Linphone_log(@"Changing rootViewController: %@ -> %@", currentViewController.name, newMainView.name);
        currentViewController = newMainView;
        LinphoneAppDelegate* delegate = (LinphoneAppDelegate*)[UIApplication sharedApplication].delegate;

        [UIView transitionWithView:delegate.window
                          duration:0.3
                           options:UIViewAnimationOptionTransitionFlipFromLeft|UIViewAnimationOptionAllowAnimatedContent
                        animations:^{
                            delegate.window.rootViewController = newMainView;
                            // when going to landscape-enabled view, we have to get the current portrait frame and orientation,
                            // because it could still have landscape-based size
                            if( nextViewOrientation != previousOrientation && newMainView == self.rotatingViewController ){
                                newMainView.view.frame = previousMainView.view.frame;
                                [newMainView.mainViewController.view setFrame:previousMainView.mainViewController.view.frame];
                                [newMainView willRotateToInterfaceOrientation:previousOrientation duration:0.3];
                                [newMainView willAnimateRotationToInterfaceOrientation:previousOrientation duration:0.3];
                                [newMainView didRotateFromInterfaceOrientation:nextViewOrientation];
                            }

                        }
                        completion:^(BOOL finished) {
						}];
    }
    return currentViewController;
}

@end

@implementation PhoneMainView

@synthesize mainViewController;
@synthesize currentView;
@synthesize statusBarBG;
@synthesize volumeView;


#pragma mark - Lifecycle Functions

- (void)initPhoneMainView {
    currentView = nil;
    inhibitedEvents = [[NSMutableArray alloc] init];
}

- (id)init {
    self = [super init];
    if (self) {
		[self initPhoneMainView];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self initPhoneMainView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initPhoneMainView];
	}
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [mainViewController release];
    [inhibitedEvents release];

    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];

    volumeView = [[MPVolumeView alloc] initWithFrame: CGRectMake(-100,-100,16,16)];
    volumeView.showsRouteButton = false;
    volumeView.userInteractionEnabled = false;

    [self.view addSubview:mainViewController.view];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

    // Set observers
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(registrationUpdate:) 
                                                 name:kLinphoneRegistrationUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceived:) 
                                                 name:kLinphoneTextReceived
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onGlobalStateChanged:)
                                                 name:kLinphoneGlobalStateUpdate
                                               object:nil];
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(batteryLevelChanged:) 
                                                 name:UIDeviceBatteryLevelDidChangeNotification
                                               object:nil];

}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];

    // Remove observers
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kLinphoneRegistrationUpdate
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                 name:kLinphoneTextReceived
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneConfiguringStateUpdate
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:NO];
    
}

-(void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
}

- (void)setVolumeHidden:(BOOL)hidden {
    // sometimes when placing a call, the volume view will appear. Inserting a
    // carefully hidden MPVolumeView into the view hierarchy will hide it
    if( hidden ){
        if ( !(volumeView.superview == self.view) ){
            [self.view addSubview:volumeView];
        }
    } else {
        if( volumeView.superview == self.view ){
            [volumeView removeFromSuperview];
        }
    }
}


- (NSUInteger)supportedInterfaceOrientations {
    if( [LinphoneManager runningOnIpad ] || [mainViewController currentViewSupportsLandscape] )
        return UIInterfaceOrientationMaskAll;
    else {
        return UIInterfaceOrientationMaskPortrait;
    }
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
    [mainViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
    [self orientationUpdate:toInterfaceOrientation];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
    [mainViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
    [mainViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (UIInterfaceOrientation)interfaceOrientation {
    return [mainViewController currentOrientation];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    [mainViewController clearCache:[RootViewManager instance].viewDescriptionStack];
}

#pragma mark - Event Functions

- (void)textReceived:(NSNotification*)notif { 
    LinphoneAddress* from = [[notif.userInfo objectForKey:@"from_address"] pointerValue];
    NSString*      callID = [notif.userInfo objectForKey:@"call-id"];
    if(from != nil) {
		[self playMessageSoundForCallID:callID];
	}
    [self updateApplicationBadgeNumber];
}

- (void)registrationUpdate:(NSNotification*)notif {
    LinphoneRegistrationState state = [[notif.userInfo objectForKey: @"state"] intValue];
    LinphoneProxyConfig *cfg = [[notif.userInfo objectForKey: @"cfg"] pointerValue];
	//Only report bad credential issue
    if (state == LinphoneRegistrationFailed
		&&[UIApplication sharedApplication].applicationState == UIApplicationStateBackground
		&& linphone_proxy_config_get_error(cfg) == LinphoneReasonBadCredentials ) {
		UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Registration failure",nil)
														message:NSLocalizedString(@"Bad credentials, check your account settings", nil)
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
											  otherButtonTitles:nil,nil];
		[error show];
		[error release];
	}
}

- (void)onGlobalStateChanged:(NSNotification*)notif {
    LinphoneGlobalState state = (LinphoneGlobalState)[[[notif userInfo] valueForKey:@"state"] integerValue];
    static BOOL already_shown = FALSE;
    if( state == LinphoneGlobalOn && !already_shown && [LinphoneManager instance].wasRemoteProvisioned ){
        LinphoneProxyConfig* conf = NULL;
        linphone_core_get_default_proxy([LinphoneManager getLc], &conf);
        if( [[LinphoneManager instance] lpConfigBoolForKey:@"show_login_view" forSection:@"app"] && conf == NULL){
            already_shown = TRUE;
            WizardViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[WizardViewController compositeViewDescription]], WizardViewController);
            if(controller != nil) {
                [controller fillDefaultValues];
            }
        }
    }
}

- (void)callUpdate:(NSNotification*)notif {
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    NSString *message = [notif.userInfo objectForKey: @"message"];
    
    bool canHideInCallView = (linphone_core_get_calls([LinphoneManager getLc]) == NULL);
    
    // Don't handle call state during incoming call view
    if([[self currentView] equal:[IncomingCallViewController compositeViewDescription]] && state != LinphoneCallError && state != LinphoneCallEnd) {
        return;
    }
    
	switch (state) {					
		case LinphoneCallIncomingReceived: 
        {
			[self displayIncomingCall:call];
			break;
        }
		case LinphoneCallOutgoingInit: 
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
        case LinphoneCallStreamsRunning:
        {
            [self changeCurrentView:[InCallViewController compositeViewDescription]];
            break;
        }
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);
            
            if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [self changeCurrentView:[InCallViewController compositeViewDescription]];
            }
            break;
        }
		case LinphoneCallError:
        {
            [self displayCallError:call message: message];
        }
		case LinphoneCallEnd: 
        {
            if (canHideInCallView) {
                // Go to dialer view
                DialerViewController *controller = DYNAMIC_CAST([self changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
                if(controller != nil) {
                    [controller setAddress:@""];
                    [controller setTransferMode:FALSE];
                }
            } else {
                [self changeCurrentView:[InCallViewController compositeViewDescription]];
			}
			break;
        }
        default:
            break;
	}
    [self updateApplicationBadgeNumber];
}


#pragma mark - 

- (void)orientationUpdate:(UIInterfaceOrientation)orientation {
    int oldLinphoneOrientation = linphone_core_get_device_rotation([LinphoneManager getLc]);
    int newRotation = 0;
    switch (orientation) {
        case UIInterfaceOrientationPortrait:
            newRotation = 0;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            newRotation = 180;
            break;
        case UIInterfaceOrientationLandscapeRight:
            newRotation = 270;
            break;
        case UIInterfaceOrientationLandscapeLeft:
            newRotation = 90;
            break;
        default:
            newRotation = oldLinphoneOrientation;
    }
    if (oldLinphoneOrientation != newRotation) {
        linphone_core_set_device_rotation([LinphoneManager getLc], newRotation);
        LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
        if (call && linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
            //Orientation has changed, must call update call
            linphone_core_update_call([LinphoneManager getLc], call, NULL);
        }
    }
}

- (void)startUp {
    LinphoneCore* core = nil;
    @try {
        core = [LinphoneManager getLc];
        LinphoneManager* lm = [LinphoneManager instance];
        if( linphone_core_get_global_state(core) != LinphoneGlobalOn ){
            [self changeCurrentView: [DialerViewController compositeViewDescription]];
        } else if ([[LinphoneManager instance] lpConfigBoolForKey:@"enable_first_login_view_preference"]  == true) {
            // Change to fist login view
            [self changeCurrentView: [FirstLoginViewController compositeViewDescription]];
        } else {
            // always start to dialer when testing
            // Change to default view
            const MSList *list = linphone_core_get_proxy_config_list(core);
            if(list != NULL || ([lm lpConfigBoolForKey:@"hide_wizard_preference"]  == true) || lm.isTesting) {
                [self changeCurrentView: [DialerViewController compositeViewDescription]];
            } else {
                WizardViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[WizardViewController compositeViewDescription]], WizardViewController);
                if(controller != nil) {
                    [controller reset];
                }
            }
        }
        [self updateApplicationBadgeNumber]; // Update Badge at startup
    }
    @catch (NSException *exception) {
        // we'll wait until the app transitions correctly
    }
}

- (void)updateApplicationBadgeNumber {
    int count = 0;
    count += linphone_core_get_missed_calls_count([LinphoneManager getLc]);
    count += [LinphoneManager unreadMessageCount];
    count += linphone_core_get_calls_nb([LinphoneManager getLc]);
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:count];
}

+ (CATransition*)getBackwardTransition {
    BOOL RTL = [LinphoneManager langageDirectionIsRTL];
    NSString* transition = RTL? kCATransitionFromRight : kCATransitionFromLeft;
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionPush];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:transition];
    
    return trans;
}

+ (CATransition*)getForwardTransition {
    BOOL RTL = [LinphoneManager langageDirectionIsRTL];
    NSString* transition = RTL? kCATransitionFromLeft : kCATransitionFromRight;
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionPush];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:transition];
    
    return trans;
}

+ (CATransition*)getTransition:(UICompositeViewDescription *)old new:(UICompositeViewDescription *)new {
    bool left = false;
    
    if([old equal:[ChatViewController compositeViewDescription]]) {
        if([new equal:[ContactsViewController compositeViewDescription]] ||
           [new equal:[DialerViewController   compositeViewDescription]] ||
           [new equal:[HistoryViewController  compositeViewDescription]]) {
            left = true;
        }
    } else if([old equal:[SettingsViewController compositeViewDescription]]) {
        if([new equal:[DialerViewController   compositeViewDescription]] ||
           [new equal:[ContactsViewController compositeViewDescription]] ||
           [new equal:[HistoryViewController  compositeViewDescription]] ||
           [new equal:[ChatViewController     compositeViewDescription]]) {
            left = true;
        }
    } else if([old equal:[DialerViewController compositeViewDescription]]) {
        if([new equal:[ContactsViewController  compositeViewDescription]] ||
           [new equal:[HistoryViewController   compositeViewDescription]]) {
            left = true;
        }
    } else if([old equal:[ContactsViewController compositeViewDescription]]) {
        if([new equal:[HistoryViewController compositeViewDescription]]) {
            left = true;
        }
    } 

    if(left) {
        return [PhoneMainView getBackwardTransition];
    } else {
        return [PhoneMainView getForwardTransition];
    }
}

+ (PhoneMainView *) instance {
    return [[RootViewManager instance] currentView];
}

- (void) showTabBar:(BOOL)show {
    [mainViewController setToolBarHidden:!show];
}

- (void) showStateBar:(BOOL)show {
    [mainViewController setStateBarHidden:!show];
}

- (void)updateStatusBar:(UICompositeViewDescription*)to_view {
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000
    // In iOS7, the app has a black background on dialer, incoming and incall, so we have to adjust the
    // status bar style for each transition to/from these views
    BOOL toLightStatus   = (to_view != NULL) && ![to_view darkBackground];
    if( !toLightStatus ) {
        // black bg: white text on black background
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];


        [UIView animateWithDuration:0.3f
                         animations:^{statusBarBG.backgroundColor = [UIColor blackColor];} ];

    } else  {
        // light bg: black text on white bg
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
        [UIView animateWithDuration:0.3f
                         animations:^{ statusBarBG.backgroundColor = [UIColor colorWithWhite:0.935 alpha:1]; }];

    }
#endif
}


- (void)fullScreen:(BOOL)enabled {
    [statusBarBG setHidden:enabled];
    [mainViewController setFullScreen:enabled];
}

- (UIViewController*)changeCurrentView:(UICompositeViewDescription *)view {
    return [self changeCurrentView:view push:FALSE];
}

- (UIViewController*)changeCurrentView:(UICompositeViewDescription*)view push:(BOOL)push {
    BOOL force = push;
    NSMutableArray* viewStack = [RootViewManager instance].viewDescriptionStack;
    if(!push ) {
        force = [viewStack count] > 1;
        [viewStack removeAllObjects];
    }
    [viewStack addObject:view];
    return [self _changeCurrentView:view transition:nil force:force];
}

- (UIViewController*)_changeCurrentView:(UICompositeViewDescription*)view transition:(CATransition*)transition force:(BOOL)force {
    [LinphoneLogger logc:LinphoneLoggerLog format:"PhoneMainView: Change current view to %@", [view name]];

    PhoneMainView* vc = [[RootViewManager instance] setViewControllerForDescription:view];

    if(force || ![view equal:vc.currentView] || vc != self) {
        if(transition == nil)
            transition = [PhoneMainView getTransition:vc.currentView new:view];
        if ([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
            [vc.mainViewController setViewTransition:transition];
        } else {
            [vc.mainViewController setViewTransition:nil];
        }
        [vc updateStatusBar:view];
        [vc.mainViewController changeView:view];
        vc->currentView = view;
    }

    //[[RootViewManager instance] setViewControllerForDescription:view];
    
    NSDictionary* mdict = [NSMutableDictionary dictionaryWithObject:vc->currentView forKey:@"view"];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneMainViewChange object:self userInfo:mdict];
    
    return [vc->mainViewController getCurrentViewController];
}

- (void)popToView:(UICompositeViewDescription*)view {
    NSMutableArray* viewStack = [RootViewManager instance].viewDescriptionStack;
    while([viewStack count] > 1 && ![[viewStack lastObject] equal:view]) {
        [viewStack removeLastObject];
    }
    [self _changeCurrentView:[viewStack lastObject] transition:[PhoneMainView getBackwardTransition] force:TRUE];
}

- (UICompositeViewDescription *)firstView {
    UICompositeViewDescription *view = nil;
    NSArray* viewStack = [RootViewManager instance].viewDescriptionStack;
   if([viewStack count]) {
        view = [viewStack objectAtIndex:0];
    }
    return view;
}

- (UIViewController*)popCurrentView {
    [LinphoneLogger logc:LinphoneLoggerLog format:"PhoneMainView: Pop view"];
    NSMutableArray* viewStack = [RootViewManager instance].viewDescriptionStack;
   if([viewStack count] > 1) {
        [viewStack removeLastObject];
        [self _changeCurrentView:[viewStack lastObject] transition:[PhoneMainView getBackwardTransition] force:TRUE];
        return [mainViewController getCurrentViewController];
    } 
    return nil;
}

- (void)displayCallError:(LinphoneCall*) call message:(NSString*) message {
    const char* lUserNameChars=linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    NSString* lMessage;
    NSString* lTitle;
    
    //get default proxy
    LinphoneProxyConfig* proxyCfg;	
    linphone_core_get_default_proxy([LinphoneManager getLc],&proxyCfg);
    if (proxyCfg == nil) {
        lMessage = NSLocalizedString(@"Please make sure your device is connected to the internet and double check your SIP account configuration in the settings.", nil);
    } else {
        lMessage = [NSString stringWithFormat : NSLocalizedString(@"Cannot call %@", nil), lUserName];
    }
    
    if (linphone_call_get_reason(call) == LinphoneReasonNotFound) {
        lMessage = [NSString stringWithFormat : NSLocalizedString(@"'%@' not registered", nil), lUserName];
    } else {
        if (message != nil) {
            lMessage = [NSString stringWithFormat : NSLocalizedString(@"%@\nReason was: %@", nil), lMessage, message];
        }
    }
    lTitle = NSLocalizedString(@"Call failed",nil);
    UIAlertView* error = [[UIAlertView alloc] initWithTitle:lTitle
                                                    message:lMessage 
                                                   delegate:nil 
                                          cancelButtonTitle:NSLocalizedString(@"Dismiss",nil) 
                                          otherButtonTitles:nil];
    [error show];
    [error release];
}

- (void)addInhibitedEvent:(id)event {
    [inhibitedEvents addObject:event];
}

- (BOOL)removeInhibitedEvent:(id)event {
    NSUInteger index = [inhibitedEvents indexOfObject:event];
    if(index != NSNotFound) {
        [inhibitedEvents removeObjectAtIndex:index];
        return TRUE;
    }
    return FALSE;
}

#pragma mark - ActionSheet Functions

- (void)playMessageSoundForCallID:(NSString*)callID {
    if ([UIApplication sharedApplication].applicationState != UIApplicationStateBackground) {
		LinphoneManager* lm = [LinphoneManager instance];
		// if the message was already received through a push notif, we don't need to ring
        if( ![lm popPushCallID:callID] ) {
            [lm playMessageSound];
        }
    }
}

- (void)displayIncomingCall:(LinphoneCall*) call{
    LinphoneCallLog* callLog = linphone_call_get_call_log(call);
    NSString* callId         = [NSString stringWithUTF8String:linphone_call_log_get_call_id(callLog)];

	if ([UIApplication sharedApplication].applicationState != UIApplicationStateBackground) {
        LinphoneManager* lm = [LinphoneManager instance];
        BOOL callIDFromPush = [lm popPushCallID:callId];
        BOOL autoAnswer     = [lm lpConfigBoolForKey:@"autoanswer_notif_preference"];

		if (callIDFromPush && autoAnswer){
			// accept call automatically
			[lm acceptCall:call];

		} else {

            IncomingCallViewController *controller = nil;
            if( ![currentView.name isEqualToString:[IncomingCallViewController compositeViewDescription].name]){
                controller = DYNAMIC_CAST([self changeCurrentView:[IncomingCallViewController compositeViewDescription] push:TRUE],IncomingCallViewController);
            } else {
                // controller is already presented, don't bother animating a transition
                controller = DYNAMIC_CAST([self.mainViewController getCurrentViewController],IncomingCallViewController);
            }
            AudioServicesPlaySystemSound(lm.sounds.vibrate);
			if(controller != nil) {
				[controller setCall:call];
				[controller setDelegate:self];
			}

		}
	}
}

- (void)batteryLevelChanged:(NSNotification*)notif {
    float level = [UIDevice currentDevice].batteryLevel;
    UIDeviceBatteryState state = [UIDevice currentDevice].batteryState;
    [LinphoneLogger log:LinphoneLoggerDebug format:@"Battery state:%d level:%.2f", state, level];
    
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    if (call && linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
        LinphoneCallAppData* callData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
        if(callData != nil) {
            if (state == UIDeviceBatteryStateUnplugged) {
                if (level <= 0.2f && !callData->batteryWarningShown) {
                    [LinphoneLogger log:LinphoneLoggerLog format:@"Battery warning"];
                    DTActionSheet *sheet = [[[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Battery is running low. Stop video ?",nil)] autorelease];
                    [sheet addCancelButtonWithTitle:NSLocalizedString(@"Continue video", nil) block:nil];
                    [sheet addDestructiveButtonWithTitle:NSLocalizedString(@"Stop video", nil) block:^() {
                        LinphoneCallParams* paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
                        // stop video
                        linphone_call_params_enable_video(paramsCopy, FALSE);
                        linphone_core_update_call([LinphoneManager getLc], call, paramsCopy);
                    }];
                    [sheet showInView:self.view];
                    callData->batteryWarningShown = TRUE;
                }
            }
            if (level > 0.2f) {
                callData->batteryWarningShown = FALSE;
            }
        }
    }
}


#pragma mark - IncomingCallDelegate Functions

- (void)incomingCallAborted:(LinphoneCall*)call {
}

- (void)incomingCallAccepted:(LinphoneCall*)call {
    [[LinphoneManager instance] acceptCall:call];
}

- (void)incomingCallDeclined:(LinphoneCall*)call {
    linphone_core_terminate_call([LinphoneManager getLc], call);
}

@end