//
//  NotificationViewController.m
//  earlyMediaUNNotificationContentExtension
//
//  Created by Tof on 29/09/2019.
//

#import "NotificationViewController.h"
#import <UserNotifications/UserNotifications.h>
#import <UserNotificationsUI/UserNotificationsUI.h>

@interface NotificationViewController () <UNNotificationContentExtension>

@property IBOutlet UILabel *label;

@end

@implementation NotificationViewController

@synthesize mediaPlayPauseButtonType;
@synthesize mediaPlayPauseButtonFrame;
@synthesize mediaPlayPauseButtonTintColor;

- (void)viewDidLoad {
    [super viewDidLoad];
	
}

static LinphoneCore *c;

static void setEarlyMediaVideoWindow(LinphoneCall *call, void *user_data) {
	NotificationViewController *thiz = (__bridge NotificationViewController *)user_data;
	thiz.videoPreview.backgroundColor = [UIColor greenColor];
	[thiz.extensionContext mediaPlayingStarted];
	linphone_core_set_native_video_window_id(c, (__bridge void *)(thiz.videoPreview));
}

static void linphone_iphone_call_state(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state,
				       const char *message) {
	NSLog(@"Call State changed :  %s %s %d",linphone_call_get_remote_address_as_string(call),message,state);
	NotificationViewController *thiz = (__bridge NotificationViewController *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc));
	if (state == LinphoneCallStateIncomingReceived) {
		linphone_call_accept_early_media(call);
		thiz.videoPreview.backgroundColor = [UIColor greenColor];
		[thiz.extensionContext mediaPlayingStarted];
		linphone_core_set_native_video_window_id(c, (__bridge void *)(thiz.videoPreview));
	}
	if (state == LinphoneCallStateReleased) {
		thiz.videoPreview.backgroundColor = [UIColor orangeColor];
	}
	thiz.uri.text = [NSString stringWithUTF8String:linphone_call_get_remote_address_as_string(call)];
	thiz.state.text = [NSString stringWithFormat:@"%s %d",message,state];
}

- (void)didReceiveNotification:(UNNotification *)notification {
	mediaPlayPauseButtonType = UNNotificationContentExtensionMediaPlayPauseButtonTypeOverlay;
	mediaPlayPauseButtonFrame = _mediaPlayPauseButton.frame;
	mediaPlayPauseButtonTintColor = [UIColor greenColor];
	
	// Read config from shared defaults
	NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:SHARED_GROUP_NAME];
	NSLog(@"Loaded configuration from app = %@ ",[defaults stringForKey:@"core_config"]);
	
	LinphoneFactory *factory = linphone_factory_get();
	LinphoneConfig *sharedConfig = linphone_factory_create_config_from_string(factory, [defaults stringForKey:@"core_config"].UTF8String);
	linphone_config_set_string(sharedConfig,"sip","root_ca",[[[NSBundle mainBundle] bundlePath] stringByAppendingString:@"/Frameworks/linphone.framework/rootca.pem"].UTF8String);
	linphone_config_clean_entry(sharedConfig, "misc", "config-uri");
	linphone_config_set_int(sharedConfig,"proxy_0","avpf",1);

	
	c = linphone_factory_create_core_with_config_3(factory, sharedConfig, NULL);
	linphone_core_disable_chat(c,LinphoneReasonNone);
	linphone_core_set_network_reachable(c, true);
	linphone_logging_service_set_log_level(linphone_logging_service_get(), LinphoneLogLevelDebug);
	
	LinphoneVideoPolicy policy;
	policy.automatically_accept = true;
	linphone_core_set_video_policy(c, &policy);
	
	
	linphone_core_enable_video_display(c, true);
	
	LinphoneCoreCbs *cbs = linphone_factory_create_core_cbs(factory);
	linphone_core_cbs_set_call_state_changed(cbs, linphone_iphone_call_state);
	linphone_core_cbs_set_user_data(cbs, (__bridge void *)(self));
	linphone_core_add_callbacks(c,cbs);
	
	linphone_core_start(c);
	linphone_core_iterate(c);
	[NSTimer scheduledTimerWithTimeInterval:0.02 repeats:YES block:^(NSTimer * _Nonnull timer) {
		linphone_core_iterate(c);
	}];
}

- (void)mediaPlay {
	self.videoPreview.hidden =NO;
}
- (void)mediaPause {
	self.videoPreview.hidden =YES;
}

@end
