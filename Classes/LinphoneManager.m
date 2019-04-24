
/* LinphoneManager.h
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/sysctl.h>

#import <AVFoundation/AVAudioSession.h>
#import <AVFoundation/AVCaptureDevice.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreTelephony/CTCallCenter.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <SystemConfiguration/CaptiveNetwork.h>
#import <SystemConfiguration/SystemConfiguration.h>

#import "LinphoneCoreSettingsStore.h"
#import "LinphoneAppDelegate.h"
#import "LinphoneManager.h"
#import "Utils/AudioHelper.h"
#import "Utils/FileTransferDelegate.h"

#include "linphone/factory.h"
#include "linphone/linphonecore_utils.h"
#include "linphone/lpconfig.h"
#include "mediastreamer2/mscommon.h"

#import "LinphoneIOSVersion.h"

#import <AVFoundation/AVAudioPlayer.h>
#import "Utils.h"
#import "PhoneMainView.h"
#import "ChatsListView.h"
#import "ChatConversationView.h"
#import <UserNotifications/UserNotifications.h>

#define LINPHONE_LOGS_MAX_ENTRY 5000

static LinphoneCore *theLinphoneCore = nil;
static LinphoneManager *theLinphoneManager = nil;

NSString *const LINPHONERC_APPLICATION_KEY = @"app";

NSString *const kLinphoneCoreUpdate = @"LinphoneCoreUpdate";
NSString *const kLinphoneDisplayStatusUpdate = @"LinphoneDisplayStatusUpdate";
NSString *const kLinphoneMessageReceived = @"LinphoneMessageReceived";
NSString *const kLinphoneTextComposeEvent = @"LinphoneTextComposeStarted";
NSString *const kLinphoneCallUpdate = @"LinphoneCallUpdate";
NSString *const kLinphoneRegistrationUpdate = @"LinphoneRegistrationUpdate";
NSString *const kLinphoneAddressBookUpdate = @"LinphoneAddressBookUpdate";
NSString *const kLinphoneMainViewChange = @"LinphoneMainViewChange";
NSString *const kLinphoneLogsUpdate = @"LinphoneLogsUpdate";
NSString *const kLinphoneSettingsUpdate = @"LinphoneSettingsUpdate";
NSString *const kLinphoneBluetoothAvailabilityUpdate = @"LinphoneBluetoothAvailabilityUpdate";
NSString *const kLinphoneConfiguringStateUpdate = @"LinphoneConfiguringStateUpdate";
NSString *const kLinphoneGlobalStateUpdate = @"LinphoneGlobalStateUpdate";
NSString *const kLinphoneNotifyReceived = @"LinphoneNotifyReceived";
NSString *const kLinphoneNotifyPresenceReceivedForUriOrTel = @"LinphoneNotifyPresenceReceivedForUriOrTel";
NSString *const kLinphoneCallEncryptionChanged = @"LinphoneCallEncryptionChanged";
NSString *const kLinphoneFileTransferSendUpdate = @"LinphoneFileTransferSendUpdate";
NSString *const kLinphoneFileTransferRecvUpdate = @"LinphoneFileTransferRecvUpdate";
NSString *const kLinphoneQRCodeFound = @"LinphoneQRCodeFound";
NSString *const kLinphoneChatCreateViewChange = @"LinphoneChatCreateViewChange";

const int kLinphoneAudioVbrCodecDefaultBitrate = 36; /*you can override this from linphonerc or linphonerc-factory*/

extern void libmsamr_init(MSFactory *factory);
extern void libmsx264_init(MSFactory *factory);
extern void libmsopenh264_init(MSFactory *factory);
extern void libmssilk_init(MSFactory *factory);
extern void libmswebrtc_init(MSFactory *factory);
extern void libmscodec2_init(MSFactory *factory);

#define FRONT_CAM_NAME                                                                                                 \
	"AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:1" /*"AV Capture: Front Camera"*/
#define BACK_CAM_NAME                                                                                                  \
	"AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:0" /*"AV Capture: Back Camera"*/

NSString *const kLinphoneOldChatDBFilename = @"chat_database.sqlite";
NSString *const kLinphoneInternalChatDBFilename = @"linphone_chats.db";

@implementation LinphoneCallAppData
- (id)init {
	if ((self = [super init])) {
		batteryWarningShown = FALSE;
		notification = nil;
		videoRequested = FALSE;
		userInfos = [[NSMutableDictionary alloc] init];
	}
	return self;
}

@end

@interface LinphoneManager ()
@property(strong, nonatomic) AVAudioPlayer *messagePlayer;
@end

@implementation LinphoneManager

@synthesize connectivity;

struct codec_name_pref_table {
	const char *name;
	int rate;
	const char *prefname;
};

struct codec_name_pref_table codec_pref_table[] = {{"speex", 8000, "speex_8k_preference"},
												   {"speex", 16000, "speex_16k_preference"},
												   {"silk", 24000, "silk_24k_preference"},
												   {"silk", 16000, "silk_16k_preference"},
												   {"amr", 8000, "amr_preference"},
												   {"gsm", 8000, "gsm_preference"},
												   {"ilbc", 8000, "ilbc_preference"},
												   {"isac", 16000, "isac_preference"},
												   {"pcmu", 8000, "pcmu_preference"},
												   {"pcma", 8000, "pcma_preference"},
												   {"g722", 8000, "g722_preference"},
												   {"g729", 8000, "g729_preference"},
												   {"mp4v-es", 90000, "mp4v-es_preference"},
												   {"h264", 90000, "h264_preference"},
												   {"h265", 90000, "h265_preference"},
												   {"vp8", 90000, "vp8_preference"},
												   {"mpeg4-generic", 16000, "aaceld_16k_preference"},
												   {"mpeg4-generic", 22050, "aaceld_22k_preference"},
												   {"mpeg4-generic", 32000, "aaceld_32k_preference"},
												   {"mpeg4-generic", 44100, "aaceld_44k_preference"},
												   {"mpeg4-generic", 48000, "aaceld_48k_preference"},
												   {"opus", 48000, "opus_preference"},
												   {"BV16", 8000, "bv16_preference"},
												   {"CODEC2", 8000, "codec2_preference"},
												   {NULL, 0, Nil}};

+ (NSString *)getPreferenceForCodec:(const char *)name withRate:(int)rate {
	int i;
	for (i = 0; codec_pref_table[i].name != NULL; ++i) {
		if (strcasecmp(codec_pref_table[i].name, name) == 0 && codec_pref_table[i].rate == rate)
			return [NSString stringWithUTF8String:codec_pref_table[i].prefname];
	}
	return Nil;
}

+ (NSSet *)unsupportedCodecs {
	NSMutableSet *set = [NSMutableSet set];
	for (int i = 0; codec_pref_table[i].name != NULL; ++i) {
		PayloadType *available = linphone_core_find_payload_type(
			theLinphoneCore, codec_pref_table[i].name, codec_pref_table[i].rate, LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS);
		if ((available == NULL)
			// these two codecs should not be hidden, even if not supported
			&& strcmp(codec_pref_table[i].prefname, "h264_preference") != 0 &&
			strcmp(codec_pref_table[i].prefname, "mp4v-es_preference") != 0) {
			[set addObject:[NSString stringWithUTF8String:codec_pref_table[i].prefname]];
		}
	}
	return set;
}

+ (BOOL)isCodecSupported:(const char *)codecName {
	return (codecName != NULL) &&
		   (NULL != linphone_core_find_payload_type(theLinphoneCore, codecName, LINPHONE_FIND_PAYLOAD_IGNORE_RATE,
													LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS));
}

+ (BOOL)runningOnIpad {
	return ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad);
}

+ (BOOL)isRunningTests {
	NSDictionary *environment = [[NSProcessInfo processInfo] environment];
	NSString *injectBundle = environment[@"XCInjectBundle"];
	return [[injectBundle pathExtension] isEqualToString:@"xctest"];
}

+ (BOOL)isNotIphone3G {
	static BOOL done = FALSE;
	static BOOL result;
	if (!done) {
		size_t size;
		sysctlbyname("hw.machine", NULL, &size, NULL, 0);
		char *machine = malloc(size);
		sysctlbyname("hw.machine", machine, &size, NULL, 0);
		NSString *platform = [[NSString alloc] initWithUTF8String:machine];
		free(machine);

		result = ![platform isEqualToString:@"iPhone1,2"];

		done = TRUE;
	}
	return result;
}

+ (NSString *)getUserAgent {
	return
		[NSString stringWithFormat:@"LinphoneIphone/%@ (Linphone/%s; Apple %@/%@)",
								   [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey],
								   linphone_core_get_version(), [UIDevice currentDevice].systemName,
								   [UIDevice currentDevice].systemVersion];
}

+ (LinphoneManager *)instance {
	@synchronized(self) {
		if (theLinphoneManager == nil) {
			theLinphoneManager = [[LinphoneManager alloc] init];
		}
	}
	return theLinphoneManager;
}

#ifdef DEBUG
+ (void)instanceRelease {
	if (theLinphoneManager != nil) {
		theLinphoneManager = nil;
	}
}
#endif

+ (BOOL)langageDirectionIsRTL {
	static NSLocaleLanguageDirection dir = NSLocaleLanguageDirectionLeftToRight;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
	  dir = [NSLocale characterDirectionForLanguage:[[NSLocale currentLocale] objectForKey:NSLocaleLanguageCode]];
	});
	return dir == NSLocaleLanguageDirectionRightToLeft;
}

#pragma mark - Lifecycle Functions

- (id)init {
	if ((self = [super init])) {
		[NSNotificationCenter.defaultCenter addObserver:self
											   selector:@selector(audioRouteChangeListenerCallback:)
												   name:AVAudioSessionRouteChangeNotification
												 object:nil];

		NSString *path = [[NSBundle mainBundle] pathForResource:@"msg" ofType:@"wav"];
		self.messagePlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL URLWithString:path] error:nil];

		_sounds.vibrate = kSystemSoundID_Vibrate;

		_logs = [[NSMutableArray alloc] init];
		_pushDict = [[NSMutableDictionary alloc] init];
		_database = NULL;
		_speakerEnabled = FALSE;
		_speakerBeforePause = FALSE;
		_bluetoothEnabled = FALSE;
		_conf = FALSE;
		_fileTransferDelegates = [[NSMutableArray alloc] init];
		_linphoneManagerAddressBookMap = [[OrderedDictionary alloc] init];
		pushCallIDs = [[NSMutableArray alloc] init];
		_isTesting = [LinphoneManager isRunningTests];
        [self migrateImportantFiles];
		[self renameDefaultSettings];
		[self copyDefaultSettings];
		[self overrideDefaultSettings];

		// set default values for first boot
		if ([self lpConfigStringForKey:@"debugenable_preference"] == nil) {
#ifdef DEBUG
			[self lpConfigSetInt:1 forKey:@"debugenable_preference"];
#else
			[self lpConfigSetInt:0 forKey:@"debugenable_preference"];
#endif
		}

		// by default if handle_content_encoding is not set, we use plain text for debug purposes only
		if ([self lpConfigStringForKey:@"handle_content_encoding" inSection:@"misc"] == nil) {
#ifdef DEBUG
			[self lpConfigSetString:@"none" forKey:@"handle_content_encoding" inSection:@"misc"];
#else
			[self lpConfigSetString:@"conflate" forKey:@"handle_content_encoding" inSection:@"misc"];
#endif
		}

		[self migrateFromUserPrefs];
        [self loadAvatar];
	}
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - AddressBookMap

- (void) setLinphoneManagerAddressBookMap:(OrderedDictionary*) addressBook{
	_linphoneManagerAddressBookMap = addressBook;
}

- (OrderedDictionary*) getLinphoneManagerAddressBookMap{
	return _linphoneManagerAddressBookMap;
}

- (void) setContactsUpdated:(BOOL) updated{
	_contactsUpdated = updated;
}
- (BOOL) getContactsUpdated{
	return _contactsUpdated;
}

#pragma deploymate push "ignored-api-availability"
- (void)silentPushFailed:(NSTimer *)timer {
	if (_silentPushCompletion) {
		LOGI(@"silentPush failed, silentPushCompletion block: %p", _silentPushCompletion);
		_silentPushCompletion(UIBackgroundFetchResultNoData);
		_silentPushCompletion = nil;
	}
}
#pragma deploymate pop

#pragma mark - Migration

- (void)migrationAllPost {
	[self migrationLinphoneSettings];
	[self migrationPerAccount];
}

- (void)migrationAllPre {
	// migrate xmlrpc URL if needed
	if ([self lpConfigBoolForKey:@"migration_xmlrpc"] == NO) {
		[self lpConfigSetString:@"https://subscribe.linphone.org:444/wizard.php"
						 forKey:@"xmlrpc_url"
					  inSection:@"assistant"];
		[self lpConfigSetString:@"sip:rls@sip.linphone.org" forKey:@"rls_uri" inSection:@"sip"];
		[self lpConfigSetBool:YES forKey:@"migration_xmlrpc"];
	}
	[self lpConfigSetBool:NO forKey:@"store_friends" inSection:@"misc"]; //so far, storing friends in files is not needed. may change in the future.
    
}

static int check_should_migrate_images(void *data, int argc, char **argv, char **cnames) {
	*((BOOL *)data) = TRUE;
	return 0;
}

- (void)migrateFromUserPrefs {
	static NSString *migration_flag = @"userpref_migration_done";

	if (_configDb == nil)
		return;

	if ([self lpConfigIntForKey:migration_flag withDefault:0]) {
		return;
	}

	NSDictionary *defaults = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
	NSArray *defaults_keys = [defaults allKeys];
	NSDictionary *values =
		@{ @"backgroundmode_preference" : @NO,
		   @"debugenable_preference" : @NO,
		   @"start_at_boot_preference" : @YES };
	BOOL shouldSync = FALSE;

	LOGI(@"%lu user prefs", (unsigned long)[defaults_keys count]);

	for (NSString *userpref in values) {
		if ([defaults_keys containsObject:userpref]) {
			LOGI(@"Migrating %@ from user preferences: %d", userpref, [[defaults objectForKey:userpref] boolValue]);
			[self lpConfigSetBool:[[defaults objectForKey:userpref] boolValue] forKey:userpref];
			[[NSUserDefaults standardUserDefaults] removeObjectForKey:userpref];
			shouldSync = TRUE;
		} else if ([self lpConfigStringForKey:userpref] == nil) {
			// no default value found in our linphonerc, we need to add them
			[self lpConfigSetBool:[[values objectForKey:userpref] boolValue] forKey:userpref];
		}
	}

	if (shouldSync) {
		LOGI(@"Synchronizing...");
		[[NSUserDefaults standardUserDefaults] synchronize];
	}
	// don't get back here in the future
	[self lpConfigSetBool:YES forKey:migration_flag];
}

- (void)migrationLinphoneSettings {
	/* AVPF migration */
	if ([self lpConfigBoolForKey:@"avpf_migration_done"] == FALSE) {
		const MSList *proxies = linphone_core_get_proxy_config_list(theLinphoneCore);
		while (proxies) {
			LinphoneProxyConfig *proxy = (LinphoneProxyConfig *)proxies->data;
			const char *addr = linphone_proxy_config_get_addr(proxy);
			// we want to enable AVPF for the proxies
			if (addr &&
				strstr(addr, [LinphoneManager.instance lpConfigStringForKey:@"domain_name"
																  inSection:@"app"
																withDefault:@"sip.linphone.org"]
								 .UTF8String) != 0) {
				LOGI(@"Migrating proxy config to use AVPF");
				linphone_proxy_config_enable_avpf(proxy, TRUE);
			}
			proxies = proxies->next;
		}
		[self lpConfigSetBool:TRUE forKey:@"avpf_migration_done"];
	}
	/* Quality Reporting migration */
	if ([self lpConfigBoolForKey:@"quality_report_migration_done"] == FALSE) {
		const MSList *proxies = linphone_core_get_proxy_config_list(theLinphoneCore);
		while (proxies) {
			LinphoneProxyConfig *proxy = (LinphoneProxyConfig *)proxies->data;
			const char *addr = linphone_proxy_config_get_addr(proxy);
			// we want to enable quality reporting for the proxies that are on linphone.org
			if (addr &&
				strstr(addr, [LinphoneManager.instance lpConfigStringForKey:@"domain_name"
																  inSection:@"app"
																withDefault:@"sip.linphone.org"]
								 .UTF8String) != 0) {
				LOGI(@"Migrating proxy config to send quality report");
				linphone_proxy_config_set_quality_reporting_collector(
					proxy, "sip:voip-metrics@sip.linphone.org;transport=tls");
				linphone_proxy_config_set_quality_reporting_interval(proxy, 180);
				linphone_proxy_config_enable_quality_reporting(proxy, TRUE);
			}
			proxies = proxies->next;
		}
		[self lpConfigSetBool:TRUE forKey:@"quality_report_migration_done"];
	}
	/* File transfer migration */
	if ([self lpConfigBoolForKey:@"file_transfer_migration_done"] == FALSE) {
		const char *newURL = "https://www.linphone.org:444/lft.php";
		LOGI(@"Migrating sharing server url from %s to %s", linphone_core_get_file_transfer_server(LC), newURL);
		linphone_core_set_file_transfer_server(LC, newURL);
		[self lpConfigSetBool:TRUE forKey:@"file_transfer_migration_done"];
	}
	
	if ([self lpConfigBoolForKey:@"lime_migration_done"] == FALSE) {
		const MSList *proxies = linphone_core_get_proxy_config_list(LC);
		while (proxies) {
			if (!strcmp(linphone_proxy_config_get_domain((LinphoneProxyConfig *)proxies->data),"sip.linphone.org")) {
				linphone_core_set_lime_x3dh_server_url(LC, "https://lime.linphone.org/lime-server/lime-server.php");
				break;
			}
			proxies = proxies->next;
		}
		[self lpConfigSetBool:TRUE forKey:@"lime_migration_done"];
	}
}

- (void)migrationPerAccount {
	const bctbx_list_t * proxies = linphone_core_get_proxy_config_list(LC);
	NSString *appDomain  = [LinphoneManager.instance lpConfigStringForKey:@"domain_name"
																inSection:@"app"
															  withDefault:@"sip.linphone.org"];
	while (proxies) {
		LinphoneProxyConfig *config = proxies->data;
		// can not create group chat without conference factory
		if (!linphone_proxy_config_get_conference_factory_uri(config)) {
			if (strcmp(appDomain.UTF8String, linphone_proxy_config_get_domain(config)) == 0) {
				linphone_proxy_config_set_conference_factory_uri(config, "sip:conference-factory@sip.linphone.org");
			}
		}
		proxies = proxies->next;
	}
	
	NSString *s = [self lpConfigStringForKey:@"pushnotification_preference"];
	if (s && s.boolValue) {
		LOGI(@"Migrating push notification per account, enabling for ALL");
		[self lpConfigSetBool:NO forKey:@"pushnotification_preference"];
		const MSList *proxies = linphone_core_get_proxy_config_list(LC);
		while (proxies) {
			linphone_proxy_config_set_ref_key(proxies->data, "push_notification");
			[self configurePushTokenForProxyConfig:proxies->data];
			proxies = proxies->next;
		}
	}
}

static void migrateWizardToAssistant(const char *entry, void *user_data) {
	LinphoneManager *thiz = (__bridge LinphoneManager *)(user_data);
	NSString *key = [NSString stringWithUTF8String:entry];
	[thiz lpConfigSetString:[thiz lpConfigStringForKey:key inSection:@"wizard"] forKey:key inSection:@"assistant"];
}

#pragma mark - Linphone Core Functions

+ (LinphoneCore *)getLc {
	if (theLinphoneCore == nil) {
		@throw([NSException exceptionWithName:@"LinphoneCoreException"
									   reason:@"Linphone core not initialized yet"
									 userInfo:nil]);
	}
	return theLinphoneCore;
}

#pragma mark Debug functions

+ (void)dumpLcConfig {
	if (theLinphoneCore) {
		LpConfig *conf = LinphoneManager.instance.configDb;
		char *config = lp_config_dump(conf);
		LOGI(@"\n%s", config);
		ms_free(config);
	}
}

#pragma mark - Logs Functions handlers
static void linphone_iphone_log_user_info(struct _LinphoneCore *lc, const char *message) {
	linphone_iphone_log_handler(NULL, ORTP_MESSAGE, message, NULL);
}
static void linphone_iphone_log_user_warning(struct _LinphoneCore *lc, const char *message) {
	linphone_iphone_log_handler(NULL, ORTP_WARNING, message, NULL);
}

#pragma mark - Display Status Functions

- (void)displayStatus:(NSString *)message {
	// Post event
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneDisplayStatusUpdate
													  object:self
													userInfo:@{
														@"message" : message
													}];
}

static void linphone_iphone_display_status(struct _LinphoneCore *lc, const char *message) {
	NSString *status = [[NSString alloc] initWithCString:message encoding:[NSString defaultCStringEncoding]];
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) displayStatus:status];
}

#pragma mark - Call State Functions

- (void)localNotifContinue:(NSTimer *)timer {
	UILocalNotification *notif = [timer userInfo];
	if (notif) {
		LOGI(@"cancelling/presenting local notif");
		[[UIApplication sharedApplication] cancelAllLocalNotifications];
		[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
	}
}

- (void)userNotifContinue:(NSTimer *)timer {
    UNNotificationContent *content = [timer userInfo];
	if (content && [UIApplication sharedApplication].applicationState != UIApplicationStateActive) {
		LOGI(@"cancelling/presenting user notif");
		UNNotificationRequest *req =
			[UNNotificationRequest requestWithIdentifier:@"call_request" content:content trigger:NULL];
		[[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:req
															   withCompletionHandler:^(NSError *_Nullable error) {
																 // Enable or disable features based on authorization.
																 if (error) {
																	 LOGD(@"Error while adding notification request :");
																	 LOGD(error.description);
																 }
															   }];
	}
}


- (void)onCall:(LinphoneCall *)call StateChanged:(LinphoneCallState)state withMessage:(const char *)message {
	// Handling wrapper
	LinphoneCallAppData *data = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
	if (!data) {
		data = [[LinphoneCallAppData alloc] init];
		linphone_call_set_user_data(call, (void *)CFBridgingRetain(data));
	}

#pragma deploymate push "ignored-api-availability"
	if (_silentPushCompletion) {
		// we were woken up by a silent push. Call the completion handler with NEWDATA
		// so that the push is notified to the user
		LOGI(@"onCall - handler %p", _silentPushCompletion);
		_silentPushCompletion(UIBackgroundFetchResultNewData);
		_silentPushCompletion = nil;
	}
#pragma deploymate pop

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	NSString *address = [FastAddressBook displayNameForAddress:addr];

	if (state == LinphoneCallIncomingReceived) {
		LinphoneCallLog *callLog = linphone_call_get_call_log(call);
		NSString *callId = [NSString stringWithUTF8String:linphone_call_log_get_call_id(callLog)];
		int index = [(NSNumber *)[_pushDict objectForKey:callId] intValue] - 1;
		LOGI(@"Decrementing index of long running task for call id : %@ with index : %d", callId, index);
		[_pushDict setValue:[NSNumber numberWithInt:index] forKey:callId];
		BOOL need_bg_task = FALSE;
		for (NSString *key in [_pushDict allKeys]) {
			int value = [(NSNumber *)[_pushDict objectForKey:key] intValue];
			if (value > 0) {
				need_bg_task = TRUE;
				break;
			}
		}
		if (pushBgTaskCall && !need_bg_task) {
			LOGI(@"Call received, stopping call background task for call-id [%@]", callId);
			[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskCall];
			pushBgTaskCall = 0;
		}
		/*first step is to re-enable ctcall center*/
		CTCallCenter *lCTCallCenter = [[CTCallCenter alloc] init];

		/*should we reject this call ?*/
		if ([lCTCallCenter currentCalls] != nil &&
			floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
			char *tmp = linphone_call_get_remote_address_as_string(call);
			if (tmp) {
				LOGI(@"Mobile call ongoing... rejecting call from [%s]", tmp);
				ms_free(tmp);
			}
			linphone_call_decline(call, LinphoneReasonBusy);
			return;
		}

		if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			if (call && (linphone_core_get_calls_nb(LC) < 2)) {
				if ([LinphoneManager.instance lpConfigBoolForKey:@"accept_early_media" inSection:@"app"] && [LinphoneManager.instance lpConfigBoolForKey:@"pref_accept_early_media"]) {
					[PhoneMainView.instance displayIncomingCall:call];
				} else {
#if !TARGET_IPHONE_SIMULATOR
					NSString *callId = [NSString stringWithUTF8String:linphone_call_log_get_call_id(linphone_call_get_call_log(call))];
					NSUUID *uuid = [NSUUID UUID];
					[LinphoneManager.instance.providerDelegate.calls setObject:callId forKey:uuid];
					[LinphoneManager.instance.providerDelegate.uuids setObject:uuid forKey:callId];
					BOOL video = ([UIApplication sharedApplication].applicationState == UIApplicationStateActive &&
								  linphone_video_activation_policy_get_automatically_accept(linphone_core_get_video_activation_policy(LC)) &&
								  linphone_call_params_video_enabled(linphone_call_get_remote_params(call)));
					[LinphoneManager.instance.providerDelegate reportIncomingCall:call withUUID:uuid handle:address video:video];
#else
					[PhoneMainView.instance displayIncomingCall:call];
#endif
				}
			} else if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive) {
				// Create a UNNotification
				UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
				content.title = NSLocalizedString(@"Incoming call", nil);
				content.body = address;
				content.sound = [UNNotificationSound soundNamed:@"notes_of_the_optimistic.caf"];
				content.categoryIdentifier = @"call_cat";
				content.userInfo = @{ @"CallId" : callId };
				UNNotificationRequest *req =
					[UNNotificationRequest requestWithIdentifier:@"call_request" content:content trigger:NULL];
				[[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:req
																	   withCompletionHandler:^(NSError *err){
																	   }];
			}
		} else {
			if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive) {
				// if (![LinphoneManager.instance popPushCallID:callId]) {
				// case where a remote notification is not already received
				// Create a new local notification
				if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
					UIMutableUserNotificationAction *answer = [[UIMutableUserNotificationAction alloc] init];
					answer.identifier = @"answer";
					answer.title = NSLocalizedString(@"Answer", nil);
					answer.activationMode = UIUserNotificationActivationModeForeground;
					answer.destructive = NO;
					answer.authenticationRequired = YES;

					UIMutableUserNotificationAction *decline = [[UIMutableUserNotificationAction alloc] init];
					decline.identifier = @"decline";
					decline.title = NSLocalizedString(@"Decline", nil);
					decline.activationMode = UIUserNotificationActivationModeBackground;
					decline.destructive = YES;
					decline.authenticationRequired = NO;

					NSArray *callactions = @[ decline, answer ];

					UIMutableUserNotificationCategory *callcat = [[UIMutableUserNotificationCategory alloc] init];
					callcat.identifier = @"incoming_call";
					[callcat setActions:callactions forContext:UIUserNotificationActionContextDefault];
					[callcat setActions:callactions forContext:UIUserNotificationActionContextMinimal];

					NSSet *categories = [NSSet setWithObjects:callcat, nil];

					UIUserNotificationSettings *set = [UIUserNotificationSettings
						settingsForTypes:(UIUserNotificationTypeAlert | UIUserNotificationTypeBadge |
										  UIUserNotificationTypeSound)
							  categories:categories];
					[[UIApplication sharedApplication] registerUserNotificationSettings:set];
					data->notification = [[UILocalNotification alloc] init];
					if (data->notification) {
						// iOS8 doesn't need the timer trick for the local notification.
						data->notification.category = @"incoming_call";
						if ([[UIDevice currentDevice].systemVersion floatValue] >= 8 &&
							[self lpConfigBoolForKey:@"repeat_call_notification"] == NO) {
							NSString *ring = ([LinphoneManager bundleFile:[self lpConfigStringForKey:@"local_ring"
																						   inSection:@"sound"]
																			  .lastPathComponent]
												  ?: [LinphoneManager bundleFile:@"notes_of_the_optimistic.caf"])
												 .lastPathComponent;
							data->notification.soundName = ring;
						} else {
							data->notification.soundName = @"shortring.caf";
							data->timer = [NSTimer scheduledTimerWithTimeInterval:5
																		   target:self
																		 selector:@selector(localNotifContinue:)
																		 userInfo:data->notification
																		  repeats:TRUE];
						}

						data->notification.repeatInterval = 0;

						data->notification.alertBody =
							[NSString stringWithFormat:NSLocalizedString(@"IC_MSG", nil), address];
						// data->notification.alertAction = NSLocalizedString(@"Answer", nil);
						data->notification.userInfo = @{ @"callId" : callId, @"timer" : [NSNumber numberWithInt:1] };
						data->notification.applicationIconBadgeNumber = 1;
						UIApplication *app = [UIApplication sharedApplication];
						LOGI([app currentUserNotificationSettings].description);
						[app presentLocalNotificationNow:data->notification];

						if (!incallBgTask) {
							incallBgTask =
								[[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
								  LOGW(@"Call cannot ring any more, too late");
								  [[UIApplication sharedApplication] endBackgroundTask:incallBgTask];
								  incallBgTask = 0;
								}];

							if (data->timer) {
								[[NSRunLoop currentRunLoop] addTimer:data->timer forMode:NSRunLoopCommonModes];
							}
						}
					}
				}
			}
		}
	}

	// we keep the speaker auto-enabled state in this static so that we don't
	// force-enable it on ICE re-invite if the user disabled it.
	static BOOL speaker_already_enabled = FALSE;

	// Disable speaker when no more call
	if ((state == LinphoneCallEnd || state == LinphoneCallError)) {
        [HistoryListTableView saveDataToUserDefaults];
		[[UIDevice currentDevice] setProximityMonitoringEnabled:FALSE];
		speaker_already_enabled = FALSE;
		if (linphone_core_get_calls_nb(theLinphoneCore) == 0) {
			[self setSpeakerEnabled:FALSE];
			[self removeCTCallCenterCb];
			// disable this because I don't find anygood reason for it: _bluetoothAvailable = FALSE;
			// furthermore it introduces a bug when calling multiple times since route may not be
			// reconfigured between cause leading to bluetooth being disabled while it should not
			_bluetoothEnabled = FALSE;
		}

		if (incallBgTask) {
			[[UIApplication sharedApplication] endBackgroundTask:incallBgTask];
			incallBgTask = 0;
		}

		if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive) {
				if (data->timer) {
					[data->timer invalidate];
					data->timer = nil;
				}
				LinphoneCallLog *UNlog = linphone_call_get_call_log(call);
                if ((UNlog == NULL
						|| linphone_call_log_get_status(UNlog) == LinphoneCallMissed
						|| linphone_call_log_get_status(UNlog) == LinphoneCallAborted
						|| linphone_call_log_get_status(UNlog) == LinphoneCallEarlyAborted)) {
					UNMutableNotificationContent *missed_content = [[UNMutableNotificationContent alloc] init];
                    missed_content.title = NSLocalizedString(@"Missed call", nil);
                    missed_content.body = address;
                    UNNotificationRequest *missed_req = [UNNotificationRequest requestWithIdentifier:@"call_request"
																							 content:missed_content
																							 trigger:NULL];
                    [UNUserNotificationCenter.currentNotificationCenter addNotificationRequest:missed_req
																		   withCompletionHandler:^(NSError *_Nullable error)
					 															{if (error) LOGD(@"Error while adding notification request : %@", error.description);}];
				}
                LinphoneProxyConfig *proxyCfg = linphone_core_get_default_proxy_config(theLinphoneCore);
                BOOL pushNotifEnabled = false;
                // handle proxy config if any
                if (proxyCfg) {
                    const char *refkey = proxyCfg ? linphone_proxy_config_get_ref_key(proxyCfg) : NULL;
                    pushNotifEnabled = (refkey && strcmp(refkey, "push_notification") == 0);
                }
                if (![LinphoneManager.instance lpConfigBoolForKey:@"backgroundmode_preference"] || pushNotifEnabled) {
                    linphone_core_set_network_reachable(LC, FALSE);
                    LinphoneManager.instance.connectivity = none;
                }
			}
            LinphoneCallLog *callLog2 = linphone_call_get_call_log(call);
            const char *call_id2 = linphone_call_log_get_call_id(callLog2);
            NSString *callId2 = call_id2
								? [NSString stringWithUTF8String:call_id2]
								: @"";
            NSUUID *uuid = (NSUUID *)[self.providerDelegate.uuids objectForKey:callId2];
            if (uuid) {
				LinphoneCall *callKit_call = (LinphoneCall *)linphone_core_get_calls(LC)
					? linphone_core_get_calls(LC)->data
					: NULL;
				const char *callKit_callId = callKit_call
					? linphone_call_log_get_call_id(linphone_call_get_call_log(callKit_call))
					: NULL;
				if (callKit_callId && !_conf) {
					// Create a CallKit call because there's not !
                    NSString *callKit_callIdNS = [NSString stringWithUTF8String:callKit_callId];
                    NSUUID *callKit_uuid = [NSUUID UUID];
                    [LinphoneManager.instance.providerDelegate.uuids setObject:callKit_uuid forKey:callKit_callIdNS];
					[LinphoneManager.instance.providerDelegate.calls setObject:callKit_callIdNS forKey:callKit_uuid];
                    NSString *address = [FastAddressBook displayNameForAddress:linphone_call_get_remote_address(callKit_call)];
					CXHandle *handle = [[CXHandle alloc] initWithType:CXHandleTypeGeneric value:address];
					CXStartCallAction *act = [[CXStartCallAction alloc] initWithCallUUID:callKit_uuid handle:handle];
                    CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
                    [LinphoneManager.instance.providerDelegate.controller requestTransaction:tr completion:^(NSError *err){}];
					[LinphoneManager.instance.providerDelegate.provider reportOutgoingCallWithUUID:callKit_uuid startedConnectingAtDate:nil];
                    [LinphoneManager.instance.providerDelegate.provider reportOutgoingCallWithUUID:callKit_uuid connectedAtDate:nil];
				}

				CXEndCallAction *act = [[CXEndCallAction alloc] initWithCallUUID:uuid];
                CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
                [LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																			  completion:^(NSError *err){}];
				
				LOGI(@"CallKit - clearing CK as call ended on uuid %@",uuid);
				[LinphoneManager.instance.providerDelegate.provider reportOutgoingCallWithUUID:uuid connectedAtDate:[NSDate date]];
				[self.providerDelegate.uuids removeObjectForKey:callId2];
				[self.providerDelegate.calls removeObjectForKey:uuid];
				[self.providerDelegate.provider reportCallWithUUID:uuid
													   endedAtDate:[NSDate date]
															reason:(state == LinphoneCallError ? CXCallEndedReasonFailed : CXCallEndedReasonRemoteEnded)];
				
				
			} else { // Can happen when Call-ID changes (Replaces header)
				if (linphone_core_get_calls_nb(LC) ==0) { // Need to clear all CK calls
					for (NSUUID *myUuid in self.providerDelegate.calls) {
						[self.providerDelegate.provider reportCallWithUUID:myUuid
															   endedAtDate:NULL
																	reason:(state == LinphoneCallError
																			? CXCallEndedReasonFailed
																			: CXCallEndedReasonRemoteEnded)];
					}
                    [self.providerDelegate.uuids removeAllObjects];
                    [self.providerDelegate.calls removeAllObjects];
				}
			}
		} else {
			if (data != nil && data->notification != nil) {
				LinphoneCallLog *log = linphone_call_get_call_log(call);
				// cancel local notif if needed
				if (data->timer) {
					[data->timer invalidate];
					data->timer = nil;
				}
                [[UIApplication sharedApplication] cancelLocalNotification:data->notification];
				data->notification = nil;

				if (log == NULL || linphone_call_log_get_status(log) == LinphoneCallMissed) {
					UILocalNotification *notification = [[UILocalNotification alloc] init];
                    notification.repeatInterval = 0;
                    notification.alertBody = [NSString stringWithFormat:
						NSLocalizedString(@"You missed a call from %@",nil), address];
					notification.alertAction = NSLocalizedString(@"Show", nil);
                    notification.userInfo = [NSDictionary dictionaryWithObject: [NSString stringWithUTF8String:linphone_call_log_get_call_id(log)]
																		forKey:@"callLog"];
					[[UIApplication sharedApplication] presentLocalNotificationNow:notification];
				}
			}
		}
        if (state == LinphoneCallError)
			[PhoneMainView.instance popCurrentView];
	}
	if (state == LinphoneCallReleased) {
		if (data != NULL) {
			linphone_call_set_user_data(call, NULL);
			CFBridgingRelease((__bridge CFTypeRef)(data));
		}
	}
    // Enable speaker when video
    if (state == LinphoneCallIncomingReceived || state == LinphoneCallOutgoingInit ||
			state == LinphoneCallConnected || state == LinphoneCallStreamsRunning) {
		if (linphone_call_params_video_enabled( linphone_call_get_current_params(call)) && !speaker_already_enabled && !_bluetoothEnabled) {
			[self setSpeakerEnabled:TRUE];
			speaker_already_enabled = TRUE;
		}
	}
	if (state == LinphoneCallStreamsRunning) {
		if (_speakerBeforePause) {
			_speakerBeforePause = FALSE;
			[self setSpeakerEnabled:TRUE];
			speaker_already_enabled = TRUE;
		}
	}
	if (state == LinphoneCallConnected && !mCallCenter) {
		/*only register CT call center CB for connected call*/
		[self setupGSMInteraction];
		[[UIDevice currentDevice] setProximityMonitoringEnabled:!(_speakerEnabled || _bluetoothEnabled)];
	}

	// Post event
    NSDictionary *dict = @{@"call" : [NSValue valueWithPointer:call],
						   @"state" : [NSNumber numberWithInt:state],
						   @"message" : [NSString stringWithUTF8String:message]};

	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate
													  object:self
													userInfo:dict];
}

static void linphone_iphone_call_state(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state,
									   const char *message) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onCall:call StateChanged:state withMessage:message];
}

#pragma mark - Transfert State Functions

static void linphone_iphone_transfer_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state) {
}

#pragma mark - Global state change

static void linphone_iphone_global_state_changed(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onGlobalStateChanged:gstate withMessage:message];
}

- (void)onGlobalStateChanged:(LinphoneGlobalState)state withMessage:(const char *)message {
	LOGI(@"onGlobalStateChanged: %d (message: %s)", state, message);

	NSDictionary *dict = [NSDictionary
		dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:state], @"state",
									 [NSString stringWithUTF8String:message ? message : ""], @"message", nil];

	// dispatch the notification asynchronously
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneGlobalStateUpdate object:self userInfo:dict];
	});
}

- (void)globalStateChangedNotificationHandler:(NSNotification *)notif {
	if ((LinphoneGlobalState)[[[notif userInfo] valueForKey:@"state"] integerValue] == LinphoneGlobalOn) {
		[self finishCoreConfiguration];
	}
}

#pragma mark - Configuring status changed

static void linphone_iphone_configuring_status_changed(LinphoneCore *lc, LinphoneConfiguringState status,
													   const char *message) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onConfiguringStatusChanged:status withMessage:message];
}

- (void)onConfiguringStatusChanged:(LinphoneConfiguringState)status withMessage:(const char *)message {
	LOGI(@"onConfiguringStatusChanged: %s %@", linphone_configuring_state_to_string(status),
		 message ? [NSString stringWithFormat:@"(message: %s)", message] : @"");
    
	NSDictionary *dict = [NSDictionary
		dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:status], @"state",
									 [NSString stringWithUTF8String:message ? message : ""], @"message", nil];

	// dispatch the notification asynchronously
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneConfiguringStateUpdate
														object:self
													  userInfo:dict];
	});
}

- (void)configuringStateChangedNotificationHandler:(NSNotification *)notif {
	_wasRemoteProvisioned = ((LinphoneConfiguringState)[[[notif userInfo] valueForKey:@"state"] integerValue] ==
							 LinphoneConfiguringSuccessful);
	if (_wasRemoteProvisioned) {
		LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
		if (cfg) {
			[self configurePushTokenForProxyConfig:cfg];
		}
	}
}

#pragma mark - Registration State Functions

- (void)onRegister:(LinphoneCore *)lc
			   cfg:(LinphoneProxyConfig *)cfg
			 state:(LinphoneRegistrationState)state
		   message:(const char *)cmessage {
	LOGI(@"New registration state: %s (message: %s)", linphone_registration_state_to_string(state), cmessage);

	LinphoneReason reason = linphone_proxy_config_get_error(cfg);
	NSString *message = nil;
	switch (reason) {
		case LinphoneReasonBadCredentials:
			message = NSLocalizedString(@"Bad credentials, check your account settings", nil);
			break;
		case LinphoneReasonNoResponse:
			message = NSLocalizedString(@"No response received from remote", nil);
			break;
		case LinphoneReasonUnsupportedContent:
			message = NSLocalizedString(@"Unsupported content", nil);
			break;
		case LinphoneReasonIOError:
			message = NSLocalizedString(
				@"Cannot reach the server: either it is an invalid address or it may be temporary down.", nil);
			break;

		case LinphoneReasonUnauthorized:
			message = NSLocalizedString(@"Operation is unauthorized because missing credential", nil);
			break;
		case LinphoneReasonNoMatch:
			message = NSLocalizedString(@"Operation could not be executed by server or remote client because it "
										@"didn't have any context for it",
										nil);
			break;
		case LinphoneReasonMovedPermanently:
			message = NSLocalizedString(@"Resource moved permanently", nil);
			break;
		case LinphoneReasonGone:
			message = NSLocalizedString(@"Resource no longer exists", nil);
			break;
		case LinphoneReasonTemporarilyUnavailable:
			message = NSLocalizedString(@"Temporarily unavailable", nil);
			break;
		case LinphoneReasonAddressIncomplete:
			message = NSLocalizedString(@"Address incomplete", nil);
			break;
		case LinphoneReasonNotImplemented:
			message = NSLocalizedString(@"Not implemented", nil);
			break;
		case LinphoneReasonBadGateway:
			message = NSLocalizedString(@"Bad gateway", nil);
			break;
		case LinphoneReasonServerTimeout:
			message = NSLocalizedString(@"Server timeout", nil);
			break;
		case LinphoneReasonNotAcceptable:
		case LinphoneReasonDoNotDisturb:
		case LinphoneReasonDeclined:
		case LinphoneReasonNotFound:
		case LinphoneReasonNotAnswered:
		case LinphoneReasonBusy:
		case LinphoneReasonNone:
		case LinphoneReasonUnknown:
			message = NSLocalizedString(@"Unknown error", nil);
			break;
	}

	// Post event
	NSDictionary *dict =
		[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:state], @"state",
												   [NSValue valueWithPointer:cfg], @"cfg", message, @"message", nil];
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneRegistrationUpdate object:self userInfo:dict];
}

static void linphone_iphone_registration_state(LinphoneCore *lc, LinphoneProxyConfig *cfg,
											   LinphoneRegistrationState state, const char *message) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onRegister:lc cfg:cfg state:state message:message];
}

#pragma mark - Auth info Function

static void linphone_iphone_popup_password_request(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method) {
	// let the wizard handle its own errors
	if ([PhoneMainView.instance currentView] != AssistantView.compositeViewDescription) {
		const char * realmC = linphone_auth_info_get_realm(auth_info);
		const char * usernameC = linphone_auth_info_get_username(auth_info);
		const char * domainC = linphone_auth_info_get_domain(auth_info);
		static UIAlertController *alertView = nil;

		// avoid having multiple popups
		[PhoneMainView.instance dismissViewControllerAnimated:YES completion:nil];

		// dont pop up if we are in background, in any case we will refresh registers when entering
		// the application again
		if ([[UIApplication sharedApplication] applicationState] != UIApplicationStateActive) {
			return;
		}

		NSString *realm = [NSString stringWithUTF8String:realmC];
		NSString *username = [NSString stringWithUTF8String:usernameC];
		NSString *domain = [NSString stringWithUTF8String:domainC];
		alertView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Authentification needed", nil)
														message:[NSString stringWithFormat:NSLocalizedString(@"Connection failed because authentication is "
																											 @"missing or invalid for %@@%@.\nYou can "
																											 @"provide password again, or check your "
																											 @"account configuration in the settings.", nil), username, realm]
												 preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		[alertView addTextFieldWithConfigurationHandler:^(UITextField *textField) {
			textField.placeholder = NSLocalizedString(@"Password", nil);
			textField.clearButtonMode = UITextFieldViewModeWhileEditing;
			textField.borderStyle = UITextBorderStyleRoundedRect;
			textField.secureTextEntry = YES;
		}];

		UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Confirm password", nil)
																 style:UIAlertActionStyleDefault
															   handler:^(UIAlertAction * action) {
																   NSString *password = alertView.textFields[0].text;
																   LinphoneAuthInfo *info =
																   linphone_auth_info_new(username.UTF8String, NULL, password.UTF8String, NULL,
																						  realm.UTF8String, domain.UTF8String);
																   linphone_core_add_auth_info(LC, info);
																   [LinphoneManager.instance refreshRegisters];
															   }];

		UIAlertAction* settingsAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Go to settings", nil)
																 style:UIAlertActionStyleDefault
															   handler:^(UIAlertAction * action) {
																   [PhoneMainView.instance changeCurrentView:SettingsView.compositeViewDescription];
															   }];

		[alertView addAction:defaultAction];
		[alertView addAction:continueAction];
		[alertView addAction:settingsAction];
		[PhoneMainView.instance presentViewController:alertView animated:YES completion:nil];
	}
}

#pragma mark - Text Received Functions

- (void)onMessageReceived:(LinphoneCore *)lc room:(LinphoneChatRoom *)room message:(LinphoneChatMessage *)msg {
#pragma deploymate push "ignored-api-availability"
	if (_silentPushCompletion) {
		// we were woken up by a silent push. Call the completion handler with NEWDATA
		// so that the push is notified to the user
		LOGI(@"onMessageReceived - handler %p", _silentPushCompletion);
		_silentPushCompletion(UIBackgroundFetchResultNewData);
		_silentPushCompletion = nil;
	}
#pragma deploymate pop
	NSString *callID = [NSString stringWithUTF8String:linphone_chat_message_get_custom_header(msg, "Call-ID")];
	const LinphoneAddress *peerAddress = linphone_chat_room_get_peer_address(room);
	NSString *from = [FastAddressBook displayNameForAddress:peerAddress];

	const LinphoneAddress *fromAddress = linphone_chat_message_get_from_address(msg);
	NSString *fromMsg = [FastAddressBook displayNameForAddress:fromAddress];

	char *peer_address = linphone_address_as_string_uri_only(peerAddress);
	NSString *peer_uri = [NSString stringWithUTF8String:peer_address];
	ms_free(peer_address);

	const LinphoneAddress *localAddress = linphone_chat_room_get_local_address(room);
	char *local_address = linphone_address_as_string_uri_only(localAddress);
	NSString *local_uri = [NSString stringWithUTF8String:local_address];
	ms_free(local_address);

	int index = [(NSNumber *)[_pushDict objectForKey:callID] intValue] - 1;
	LOGI(@"Decrementing index of long running task for call id : %@ with index : %d", callID, index);
	[_pushDict setValue:[NSNumber numberWithInt:index] forKey:callID];
	BOOL need_bg_task = FALSE;
	for (NSString *key in [_pushDict allKeys]) {
		int value = [(NSNumber *)[_pushDict objectForKey:key] intValue];
		if (value > 0) {
			need_bg_task = TRUE;
			break;
		}
	}
	if (pushBgTaskMsg && !need_bg_task) {
		LOGI(@"Message received, stopping message background task for call-id [%@]", callID);
		[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskMsg];
		pushBgTaskMsg = 0;
	}
    
    BOOL hasFile = FALSE;
    // if auto_download is available and file is downloaded
    if ((linphone_core_get_max_size_for_auto_download_incoming_files(LC) > -1) && linphone_chat_message_get_file_transfer_information(msg))
        hasFile = TRUE;

	if (!linphone_chat_message_is_file_transfer(msg) && !linphone_chat_message_is_text(msg) && !hasFile)
		return;
    
    if (hasFile) {
        ChatConversationView *view = VIEW(ChatConversationView);
        [view autoDownload:msg view:nil];
    }

	// Post event
	NSDictionary *dict = @{
						   @"room" : [NSValue valueWithPointer:room],
						   @"from_address" : [NSValue valueWithPointer:linphone_chat_message_get_from_address(msg)],
						   @"message" : [NSValue valueWithPointer:msg],
						   @"call-id" : callID
						   };

	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:self userInfo:dict];

	if (linphone_chat_message_is_outgoing(msg))
		return;

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		if ((PhoneMainView.instance.currentView == ChatsListView.compositeViewDescription))
			return;

		if (PhoneMainView.instance.currentView == ChatConversationView.compositeViewDescription && room == PhoneMainView.instance.currentRoom)
			return;
	}

	// Create a new notification
	if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
		NSArray *actions;
		if ([[UIDevice.currentDevice systemVersion] floatValue] < 9 ||
			[LinphoneManager.instance lpConfigBoolForKey:@"show_msg_in_notif"] == NO) {

			UIMutableUserNotificationAction *reply = [[UIMutableUserNotificationAction alloc] init];
			reply.identifier = @"reply";
			reply.title = NSLocalizedString(@"Reply", nil);
			reply.activationMode = UIUserNotificationActivationModeForeground;
			reply.destructive = NO;
			reply.authenticationRequired = YES;

			UIMutableUserNotificationAction *mark_read = [[UIMutableUserNotificationAction alloc] init];
			mark_read.identifier = @"mark_read";
			mark_read.title = NSLocalizedString(@"Mark Read", nil);
			mark_read.activationMode = UIUserNotificationActivationModeBackground;
			mark_read.destructive = NO;
			mark_read.authenticationRequired = NO;

			actions = @[ mark_read, reply ];
		} else {
			// iOS 9 allows for inline reply. We don't propose mark_read in this case
			UIMutableUserNotificationAction *reply_inline = [[UIMutableUserNotificationAction alloc] init];

			reply_inline.identifier = @"reply_inline";
			reply_inline.title = NSLocalizedString(@"Reply", nil);
			reply_inline.activationMode = UIUserNotificationActivationModeBackground;
			reply_inline.destructive = NO;
			reply_inline.authenticationRequired = NO;
			reply_inline.behavior = UIUserNotificationActionBehaviorTextInput;

			actions = @[ reply_inline ];
		}

		UIMutableUserNotificationCategory *msgcat = [[UIMutableUserNotificationCategory alloc] init];
		msgcat.identifier = @"incoming_msg";
		[msgcat setActions:actions forContext:UIUserNotificationActionContextDefault];
		[msgcat setActions:actions forContext:UIUserNotificationActionContextMinimal];

		NSSet *categories = [NSSet setWithObjects:msgcat, nil];

		UIUserNotificationSettings *set = [UIUserNotificationSettings settingsForTypes:(UIUserNotificationTypeAlert | UIUserNotificationTypeBadge | UIUserNotificationTypeSound)
																			categories:categories];
		[[UIApplication sharedApplication] registerUserNotificationSettings:set];

		UILocalNotification *notif = [[UILocalNotification alloc] init];
		if (notif) {
			NSString *chat = [UIChatBubbleTextCell TextMessageForChat:msg];
			notif.repeatInterval = 0;
			if ([[UIDevice currentDevice].systemVersion floatValue] >= 8) {
#pragma deploymate push "ignored-api-availability"
				notif.category = @"incoming_msg";
#pragma deploymate pop
			}
			if ([LinphoneManager.instance lpConfigBoolForKey:@"show_msg_in_notif" withDefault:YES]) {
				notif.alertBody = [NSString stringWithFormat:NSLocalizedString(@"IM_FULLMSG", nil), from, chat];
			} else {
				notif.alertBody = [NSString stringWithFormat:NSLocalizedString(@"IM_MSG", nil), from];
			}
			notif.alertAction = NSLocalizedString(@"Show", nil);
			notif.soundName = @"msg.caf";
			notif.userInfo = @{@"from" : from, @"peer_addr" : peer_uri, @"local_addr" : local_uri, @"call-id" : callID};
			notif.accessibilityLabel = @"Message notif";
			[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
		}
	} else {
		UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
		content.title = NSLocalizedString(@"Message received", nil);
		const char* subject = linphone_chat_room_get_subject(room) ?: LINPHONE_DUMMY_SUBJECT;
		if ([LinphoneManager.instance lpConfigBoolForKey:@"show_msg_in_notif" withDefault:YES]) {
			content.subtitle = strcmp(subject, LINPHONE_DUMMY_SUBJECT) != 0 ? [NSString stringWithUTF8String:subject] : fromMsg;
			content.body = strcmp(subject, LINPHONE_DUMMY_SUBJECT) != 0
				? [NSString stringWithFormat:@"%@ : %@", fromMsg, [UIChatBubbleTextCell TextMessageForChat:msg]]
				: [UIChatBubbleTextCell TextMessageForChat:msg];
		} else {
			content.body = strcmp(subject, LINPHONE_DUMMY_SUBJECT) != 0
				? [NSString stringWithFormat:@"%@ : %@",[NSString stringWithUTF8String:subject], fromMsg]
				: fromMsg;
		}
		content.sound = [UNNotificationSound soundNamed:@"msg.caf"];
		content.categoryIdentifier = @"msg_cat";
        // save data to user info for rich notification content
        NSMutableArray *msgs = [NSMutableArray array];
        bctbx_list_t *history = linphone_chat_room_get_history(room, 6);
        while (history) {
            NSMutableDictionary *msgData = [NSMutableDictionary dictionary];
            LinphoneChatMessage *msg = history->data;
            const char *state = linphone_chat_message_state_to_string(linphone_chat_message_get_state(msg));
            bool_t isOutgoing = linphone_chat_message_is_outgoing(msg);
            bool_t isFileTransfer = (linphone_chat_message_get_file_transfer_information(msg) != NULL);
            const LinphoneAddress *fromAddress = linphone_chat_message_get_from_address(msg);
            NSString *displayNameDate = [NSString stringWithFormat:@"%@ - %@", [LinphoneUtils timeToString:linphone_chat_message_get_time(msg)
                                                                                                withFormat:LinphoneDateChatBubble],
                                         [FastAddressBook displayNameForAddress:fromAddress]];
            UIImage *fromImage = [UIImage resizeImage:[FastAddressBook imageForAddress:fromAddress]
                                         withMaxWidth:200
                                         andMaxHeight:200];
            NSData *fromImageData = UIImageJPEGRepresentation(fromImage, 1);
            [msgData setObject:[NSString stringWithUTF8String:state] forKey:@"state"];
            [msgData setObject:displayNameDate forKey:@"displayNameDate"];
            [msgData setObject:[NSNumber numberWithBool:isFileTransfer] forKey:@"isFileTransfer"];
            [msgData setObject:fromImageData forKey:@"fromImageData"];
            if (isFileTransfer) {
                LinphoneContent *file = linphone_chat_message_get_file_transfer_information(msg);
                const char *filename = linphone_content_get_name(file);
                [msgData setObject:[NSString stringWithUTF8String:filename] forKey:@"msg"];
            } else {
                [msgData setObject:[UIChatBubbleTextCell TextMessageForChat:msg] forKey:@"msg"];
            }
            [msgData setObject:[NSNumber numberWithBool:isOutgoing] forKey:@"isOutgoing"];
            [msgs addObject:msgData];
            history = bctbx_list_next(history);
        }
        content.userInfo = @{@"from" : from, @"peer_addr" : peer_uri, @"local_addr" : local_uri, @"CallId" : callID, @"msgs" : msgs};
		content.accessibilityLabel = @"Message notif";
		UNNotificationRequest *req = [UNNotificationRequest requestWithIdentifier:@"call_request" content:content trigger:NULL];
		[[UNUserNotificationCenter currentNotificationCenter]
			addNotificationRequest:req
			 withCompletionHandler:^(NSError *_Nullable error) {
			   // Enable or disable features based on authorization.
			   if (error) {
				   LOGD(@"Error while adding notification request :");
				   LOGD(error.description);
			   }
			 }];
	}
    [ChatsListTableView saveDataToUserDefaults];
    [HistoryListTableView saveDataToUserDefaults];
}

static void linphone_iphone_message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onMessageReceived:lc room:room message:message];
}

static void linphone_iphone_message_received_unable_decrypt(LinphoneCore *lc, LinphoneChatRoom *room,
															LinphoneChatMessage *message) {

	NSString *callId = [NSString stringWithUTF8String:linphone_chat_message_get_custom_header(message, "Call-ID")];
	int index = [(NSNumber *)[LinphoneManager.instance.pushDict objectForKey:callId] intValue] - 1;
	LOGI(@"Decrementing index of long running task for call id : %@ with index : %d", callId, index);
	[LinphoneManager.instance.pushDict setValue:[NSNumber numberWithInt:index] forKey:callId];
	BOOL need_bg_task = FALSE;
	for (NSString *key in [LinphoneManager.instance.pushDict allKeys]) {
		int value = [(NSNumber *)[LinphoneManager.instance.pushDict objectForKey:key] intValue];
		if (value > 0) {
			need_bg_task = TRUE;
			break;
		}
	}
	if (theLinphoneManager->pushBgTaskMsg && !need_bg_task) {
		LOGI(@"Message received, stopping message background task for call-id [%@]", callId);
		[[UIApplication sharedApplication] endBackgroundTask:theLinphoneManager->pushBgTaskMsg];
		theLinphoneManager->pushBgTaskMsg = 0;
	}
	const LinphoneAddress *address = linphone_chat_message_get_peer_address(message);
	NSString *strAddr = [FastAddressBook displayNameForAddress:address];
	NSString *title = NSLocalizedString(@"LIME warning", nil);
	NSString *body = [NSString
		stringWithFormat:NSLocalizedString(@"You have received an encrypted message you are unable to decrypt from "
										   @"%@.\nYou need to call your correspondant in order to exchange your ZRTP "
										   @"keys if you want to decrypt the future messages you will receive.",
										   nil),
						 strAddr];
	NSString *action = NSLocalizedString(@"Call", nil);

	if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive) {
		if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
			content.title = title;
			content.body = body;
			UNNotificationRequest *req =
				[UNNotificationRequest requestWithIdentifier:@"decrypt_request" content:content trigger:NULL];
			[[UNUserNotificationCenter currentNotificationCenter]
				addNotificationRequest:req
				 withCompletionHandler:^(NSError *_Nullable error) {
				   // Enable or disable features based on authorization.
				   if (error) {
					   LOGD(@"Error while adding notification request :");
					   LOGD(error.description);
				   }
				 }];
		} else {
			UILocalNotification *notification = [[UILocalNotification alloc] init];
			notification.repeatInterval = 0;
			notification.alertTitle = title;
			notification.alertBody = body;
			[[UIApplication sharedApplication] presentLocalNotificationNow:notification];
		}
	} else {
		UIAlertController *errView =
			[UIAlertController alertControllerWithTitle:title message:body preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction *action){
															  }];

		UIAlertAction *callAction = [UIAlertAction actionWithTitle:action
															 style:UIAlertActionStyleDefault
														   handler:^(UIAlertAction *action) {
															 [LinphoneManager.instance call:address];
														   }];

		[errView addAction:defaultAction];
		[errView addAction:callAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
	}
}

- (void)onNotifyReceived:(LinphoneCore *)lc
				   event:(LinphoneEvent *)lev
			 notifyEvent:(const char *)notified_event
				 content:(const LinphoneContent *)body {
	// Post event
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	[dict setObject:[NSValue valueWithPointer:lev] forKey:@"event"];
	[dict setObject:[NSString stringWithUTF8String:notified_event] forKey:@"notified_event"];
	if (body != NULL) {
		[dict setObject:[NSValue valueWithPointer:body] forKey:@"content"];
	}
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneNotifyReceived object:self userInfo:dict];
}

static void linphone_iphone_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event,
											const LinphoneContent *body) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onNotifyReceived:lc
																			event:lev
																	  notifyEvent:notified_event
																		  content:body];
}

- (void)onNotifyPresenceReceivedForUriOrTel:(LinphoneCore *)lc
									 friend:(LinphoneFriend *)lf
										uri:(const char *)uri
							  presenceModel:(const LinphonePresenceModel *)model {
	// Post event
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	[dict setObject:[NSValue valueWithPointer:lf] forKey:@"friend"];
	[dict setObject:[NSValue valueWithPointer:uri] forKey:@"uri"];
	[dict setObject:[NSValue valueWithPointer:model] forKey:@"presence_model"];
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneNotifyPresenceReceivedForUriOrTel
													  object:self
													userInfo:dict];
}

static void linphone_iphone_notify_presence_received_for_uri_or_tel(LinphoneCore *lc, LinphoneFriend *lf,
																	const char *uri_or_tel,
																	const LinphonePresenceModel *presence_model) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onNotifyPresenceReceivedForUriOrTel:lc
																							  friend:lf
																								 uri:uri_or_tel
																					   presenceModel:presence_model];
}

static void linphone_iphone_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t on,
													const char *authentication_token) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onCallEncryptionChanged:lc
																					call:call
																					  on:on
																				   token:authentication_token];
}

- (void)onCallEncryptionChanged:(LinphoneCore *)lc
						   call:(LinphoneCall *)call
							 on:(BOOL)on
						  token:(const char *)authentication_token {
	// Post event
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	[dict setObject:[NSValue valueWithPointer:call] forKey:@"call"];
	[dict setObject:[NSNumber numberWithBool:on] forKey:@"on"];
	if (authentication_token) {
		[dict setObject:[NSString stringWithUTF8String:authentication_token] forKey:@"token"];
	}
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallEncryptionChanged object:self userInfo:dict];
}

void linphone_iphone_chatroom_state_changed(LinphoneCore *lc, LinphoneChatRoom *cr, LinphoneChatRoomState state) {
    if (state == LinphoneChatRoomStateCreated) {
        [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:nil];
    }
}

void linphone_iphone_version_update_check_result_received (LinphoneCore *lc, LinphoneVersionUpdateCheckResult result, const char *version, const char *url) {
    if (result == LinphoneVersionUpdateCheckUpToDate || result == LinphoneVersionUpdateCheckError) {
        return;
    }
    NSString *title = NSLocalizedString(@"Outdated Version", nil);
    NSString *body = NSLocalizedString(@"A new version of your app is available, use the button below to download it.", nil);

    UIAlertController *versVerifView = [UIAlertController alertControllerWithTitle:title
                                                                     message:body
                                                              preferredStyle:UIAlertControllerStyleAlert];

    NSString *ObjCurl = [NSString stringWithUTF8String:url];
    UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Download", nil)
                                                            style:UIAlertActionStyleDefault
                                                          handler:^(UIAlertAction * action) {
                                                              [[UIApplication sharedApplication] openURL:[NSURL URLWithString:ObjCurl]];
                                                          }];

    [versVerifView addAction:defaultAction];
    [PhoneMainView.instance presentViewController:versVerifView animated:YES completion:nil];
}

void linphone_iphone_qr_code_found(LinphoneCore *lc, const char *result) {
    NSDictionary *eventDic = [NSDictionary dictionaryWithObject:[NSString stringWithCString:result encoding:[NSString defaultCStringEncoding]] forKey:@"qrcode"];
    LOGD(@"QRCODE FOUND");
    [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneQRCodeFound object:nil userInfo:eventDic];
}

#pragma mark - Message composition start
- (void)alertLIME:(LinphoneChatRoom *)room {
	NSString *title = NSLocalizedString(@"LIME warning", nil);
	NSString *body =
		NSLocalizedString(@"You are trying to send a message using LIME to a contact not verified by ZRTP.\n"
						  @"Please call this contact and verify his ZRTP key before sending your messages.",
						  nil);

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
		UIAlertController *errView =
			[UIAlertController alertControllerWithTitle:title message:body preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction *action){}];
		[errView addAction:defaultAction];

		UIAlertAction *callAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Call", nil)
															 style:UIAlertActionStyleDefault
														   handler:^(UIAlertAction *action) {
															 [self call:linphone_chat_room_get_peer_address(room)];
														   }];
		[errView addAction:callAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
	} else {
		if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
			content.title = title;
			content.body = body;
			content.categoryIdentifier = @"lime";

			UNNotificationRequest *req = [UNNotificationRequest
				requestWithIdentifier:@"lime_request"
							  content:content
							  trigger:[UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO]];
			[[UNUserNotificationCenter currentNotificationCenter]
				addNotificationRequest:req
				 withCompletionHandler:^(NSError *_Nullable error) {
				   // Enable or disable features based on authorization.
				   if (error) {
					   LOGD(@"Error while adding notification request :");
					   LOGD(error.description);
				   }
				 }];
		} else {
			UILocalNotification *notification = [[UILocalNotification alloc] init];
			notification.repeatInterval = 0;
			notification.alertTitle = title;
			notification.alertBody = body;
			[[UIApplication sharedApplication] presentLocalNotificationNow:notification];
		}
	}
}

- (void)onMessageComposeReceived:(LinphoneCore *)core forRoom:(LinphoneChatRoom *)room {
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneTextComposeEvent
													  object:self
													userInfo:@{
														@"room" : [NSValue valueWithPointer:room]
													}];
}

static void linphone_iphone_is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
	[(__bridge LinphoneManager *)linphone_core_cbs_get_user_data(linphone_core_get_current_callbacks(lc)) onMessageComposeReceived:lc forRoom:room];
}

#pragma mark - Network Functions

- (SCNetworkReachabilityRef)getProxyReachability {
	return proxyReachability;
}

+ (void)kickOffNetworkConnection {
	static BOOL in_progress = FALSE;
	if (in_progress) {
		LOGW(@"Connection kickoff already in progress");
		return;
	}
	in_progress = TRUE;
	/* start a new thread to avoid blocking the main ui in case of peer host failure */
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
	  static int sleep_us = 10000;
	  static int timeout_s = 5;
	  BOOL timeout_reached = FALSE;
	  int loop = 0;
	  CFWriteStreamRef writeStream;
	  CFStreamCreatePairWithSocketToHost(NULL, (CFStringRef) @"192.168.0.200" /*"linphone.org"*/, 15000, nil,
										 &writeStream);
	  BOOL res = CFWriteStreamOpen(writeStream);
	  const char *buff = "hello";
	  time_t start = time(NULL);
	  time_t loop_time;

	  if (res == FALSE) {
		  LOGI(@"Could not open write stream, backing off");
		  CFRelease(writeStream);
		  in_progress = FALSE;
		  return;
	  }

	  // check stream status and handle timeout
	  CFStreamStatus status = CFWriteStreamGetStatus(writeStream);
	  while (status != kCFStreamStatusOpen && status != kCFStreamStatusError) {
		  usleep(sleep_us);
		  status = CFWriteStreamGetStatus(writeStream);
		  loop_time = time(NULL);
		  if (loop_time - start >= timeout_s) {
			  timeout_reached = TRUE;
			  break;
		  }
		  loop++;
	  }

	  if (status == kCFStreamStatusOpen) {
		  CFWriteStreamWrite(writeStream, (const UInt8 *)buff, strlen(buff));
	  } else if (!timeout_reached) {
		  CFErrorRef error = CFWriteStreamCopyError(writeStream);
		  LOGD(@"CFStreamError: %@", error);
		  CFRelease(error);
	  } else if (timeout_reached) {
		  LOGI(@"CFStream timeout reached");
	  }
	  CFWriteStreamClose(writeStream);
	  CFRelease(writeStream);
	  in_progress = FALSE;
	});
}

+ (NSString *)getCurrentWifiSSID {
#if TARGET_IPHONE_SIMULATOR
	return @"Sim_err_SSID_NotSupported";
#else
	NSString *data = nil;
	CFDictionaryRef dict = CNCopyCurrentNetworkInfo((CFStringRef) @"en0");
	if (dict) {
		LOGI(@"AP Wifi: %@", dict);
		data = [NSString stringWithString:(NSString *)CFDictionaryGetValue(dict, @"SSID")];
		CFRelease(dict);
	}
	return data;
#endif
}

static void showNetworkFlags(SCNetworkReachabilityFlags flags) {
	NSMutableString *log = [[NSMutableString alloc] initWithString:@"Network connection flags: "];
	if (flags == 0)
		[log appendString:@"no flags."];
	if (flags & kSCNetworkReachabilityFlagsTransientConnection)
		[log appendString:@"kSCNetworkReachabilityFlagsTransientConnection, "];
	if (flags & kSCNetworkReachabilityFlagsReachable)
		[log appendString:@"kSCNetworkReachabilityFlagsReachable, "];
	if (flags & kSCNetworkReachabilityFlagsConnectionRequired)
		[log appendString:@"kSCNetworkReachabilityFlagsConnectionRequired, "];
	if (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)
		[log appendString:@"kSCNetworkReachabilityFlagsConnectionOnTraffic, "];
	if (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)
		[log appendString:@"kSCNetworkReachabilityFlagsConnectionOnDemand, "];
	if (flags & kSCNetworkReachabilityFlagsIsLocalAddress)
		[log appendString:@"kSCNetworkReachabilityFlagsIsLocalAddress, "];
	if (flags & kSCNetworkReachabilityFlagsIsDirect)
		[log appendString:@"kSCNetworkReachabilityFlagsIsDirect, "];
	if (flags & kSCNetworkReachabilityFlagsIsWWAN)
		[log appendString:@"kSCNetworkReachabilityFlagsIsWWAN, "];
	LOGI(@"%@", log);
}

//This callback keeps tracks of wifi SSID changes.
static void networkReachabilityNotification(CFNotificationCenterRef center, void *observer, CFStringRef name,
											const void *object, CFDictionaryRef userInfo) {
	LinphoneManager *mgr = LinphoneManager.instance;
	SCNetworkReachabilityFlags flags;

	// for an unknown reason, we are receiving multiple time the notification, so
	// we will skip each time the SSID did not change
	NSString *newSSID = [LinphoneManager getCurrentWifiSSID];
	if ([newSSID compare:mgr.SSID] == NSOrderedSame)
		return;


	if (newSSID != Nil && newSSID.length > 0 && mgr.SSID != Nil && newSSID.length > 0) {
		if (SCNetworkReachabilityGetFlags([mgr getProxyReachability], &flags)) {
			LOGI(@"Wifi SSID changed, resesting transports.");
			mgr.connectivity=none; //this will trigger a connectivity change in networkReachabilityCallback.
			networkReachabilityCallBack([mgr getProxyReachability], flags, nil);
		}
	}
	mgr.SSID = newSSID;
}

void networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void *nilCtx) {
	showNetworkFlags(flags);
	LinphoneManager *lm = LinphoneManager.instance;
	SCNetworkReachabilityFlags networkDownFlags = kSCNetworkReachabilityFlagsConnectionRequired |
												  kSCNetworkReachabilityFlagsConnectionOnTraffic |
												  kSCNetworkReachabilityFlagsConnectionOnDemand;

	if (theLinphoneCore != nil) {
		LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(theLinphoneCore);

		struct NetworkReachabilityContext *ctx = nilCtx ? ((struct NetworkReachabilityContext *)nilCtx) : 0;
		if ((flags == 0) || (flags & networkDownFlags)) {
			linphone_core_set_network_reachable(theLinphoneCore, false);
			lm.connectivity = none;
			[LinphoneManager kickOffNetworkConnection];
		} else {
			Connectivity newConnectivity;
			BOOL isWifiOnly = [lm lpConfigBoolForKey:@"wifi_only_preference" withDefault:FALSE];
			if (!ctx || ctx->testWWan)
				newConnectivity = flags & kSCNetworkReachabilityFlagsIsWWAN ? wwan : wifi;
			else
				newConnectivity = wifi;

			if (newConnectivity == wwan && proxy && isWifiOnly &&
				(lm.connectivity == newConnectivity || lm.connectivity == none)) {
				linphone_proxy_config_expires(proxy, 0);
			} else if (proxy) {
				NSInteger defaultExpire = [lm lpConfigIntForKey:@"default_expires"];
				if (defaultExpire >= 0)
					linphone_proxy_config_expires(proxy, (int)defaultExpire);
				// else keep default value from linphonecore
			}

			if (lm.connectivity != newConnectivity) {
				// connectivity has changed
				linphone_core_set_network_reachable(theLinphoneCore, false);
				if (newConnectivity == wwan && proxy && isWifiOnly) {
					linphone_proxy_config_expires(proxy, 0);
				}
				linphone_core_set_network_reachable(theLinphoneCore, true);
				[LinphoneManager.instance iterate];
				LOGI(@"Network connectivity changed to type [%s]", (newConnectivity == wifi ? "wifi" : "wwan"));
				lm.connectivity = newConnectivity;
			}
		}
		if (ctx && ctx->networkStateChanged) {
			(*ctx->networkStateChanged)(lm.connectivity);
		}
	}
}

- (void)setupNetworkReachabilityCallback {
	SCNetworkReachabilityContext *ctx = NULL;
	// any internet cnx
	struct sockaddr_in zeroAddress;
	bzero(&zeroAddress, sizeof(zeroAddress));
	zeroAddress.sin_len = sizeof(zeroAddress);
	zeroAddress.sin_family = AF_INET;

	if (proxyReachability) {
		LOGI(@"Cancelling old network reachability");
		SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		CFRelease(proxyReachability);
		proxyReachability = nil;
	}

	// This notification is used to detect SSID change (switch of Wifi network). The ReachabilityCallback is
	// not triggered when switching between 2 private Wifi...
	// Since we cannot be sure we were already observer, remove ourself each time... to be improved
	_SSID = [LinphoneManager getCurrentWifiSSID];
	CFNotificationCenterRemoveObserver(CFNotificationCenterGetDarwinNotifyCenter(), (__bridge const void *)(self),
									   CFSTR("com.apple.system.config.network_change"), NULL);
	CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), (__bridge const void *)(self),
									networkReachabilityNotification, CFSTR("com.apple.system.config.network_change"),
									NULL, CFNotificationSuspensionBehaviorDeliverImmediately);

	proxyReachability =
		SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr *)&zeroAddress);

	if (!SCNetworkReachabilitySetCallback(proxyReachability, (SCNetworkReachabilityCallBack)networkReachabilityCallBack,
										  ctx)) {
		LOGE(@"Cannot register reachability cb: %s", SCErrorString(SCError()));
		return;
	}
	if (!SCNetworkReachabilityScheduleWithRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode)) {
		LOGE(@"Cannot register schedule reachability cb: %s", SCErrorString(SCError()));
		return;
	}

	// this check is to know network connectivity right now without waiting for a change. Don'nt remove it unless you
	// have good reason. Jehan
	SCNetworkReachabilityFlags flags;
	if (SCNetworkReachabilityGetFlags(proxyReachability, &flags)) {
		networkReachabilityCallBack(proxyReachability, flags, nil);
	}
}

- (NetworkType)network {
	if ([[[UIDevice currentDevice] systemVersion] floatValue] < 7) {
		UIApplication *app = [UIApplication sharedApplication];
		NSArray *subviews = [[[app valueForKey:@"statusBar"] valueForKey:@"foregroundView"] subviews];
		NSNumber *dataNetworkItemView = nil;

		for (id subview in subviews) {
			if ([subview isKindOfClass:[NSClassFromString(@"UIStatusBarDataNetworkItemView") class]]) {
				dataNetworkItemView = subview;
				break;
			}
		}

		NSNumber *number = (NSNumber *)[dataNetworkItemView valueForKey:@"dataNetworkType"];
		return [number intValue];
	} else {
#pragma deploymate push "ignored-api-availability"
		CTTelephonyNetworkInfo *info = [[CTTelephonyNetworkInfo alloc] init];
		NSString *currentRadio = info.currentRadioAccessTechnology;
		if ([currentRadio isEqualToString:CTRadioAccessTechnologyEdge]) {
			return network_2g;
		} else if ([currentRadio isEqualToString:CTRadioAccessTechnologyLTE]) {
			return network_4g;
		}
#pragma deploymate pop
		return network_3g;
	}
}

#pragma mark -

// scheduling loop
- (void)iterate {
	UIBackgroundTaskIdentifier coreIterateTaskId = 0;
	coreIterateTaskId = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
		LOGW(@"Background task for core iteration launching expired.");
		[[UIApplication sharedApplication] endBackgroundTask:coreIterateTaskId];
	}];
	linphone_core_iterate(theLinphoneCore);
	if (coreIterateTaskId != UIBackgroundTaskInvalid)
		[[UIApplication sharedApplication] endBackgroundTask:coreIterateTaskId];
}

- (void)audioSessionInterrupted:(NSNotification *)notification {
	int interruptionType = [notification.userInfo[AVAudioSessionInterruptionTypeKey] intValue];
	if (interruptionType == AVAudioSessionInterruptionTypeBegan) {
		[self beginInterruption];
	} else if (interruptionType == AVAudioSessionInterruptionTypeEnded) {
		[self endInterruption];
	}
}

/** Should be called once per linphone_core_new() */
- (void)finishCoreConfiguration {
	//Force keep alive to workaround push notif on chat message
	linphone_core_enable_keep_alive(theLinphoneCore, true);

	// get default config from bundle
	NSString *zrtpSecretsFileName = [LinphoneManager dataFile:@"zrtp_secrets"];
	NSString *chatDBFileName = [LinphoneManager dataFile:kLinphoneInternalChatDBFilename];

	NSString *device = [[NSMutableString alloc]
		initWithString:[NSString
						stringWithFormat:@"%@iOS/%@ (%@) LinphoneSDK",
						[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleDisplayName"],
						[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
						[[UIDevice currentDevice] name]]];

	linphone_core_set_user_agent(theLinphoneCore, device.UTF8String, LINPHONE_SDK_VERSION);

	_contactSipField = [self lpConfigStringForKey:@"contact_im_type_value" inSection:@"sip" withDefault:@"SIP"];

	if (_fastAddressBook == nil) {
		_fastAddressBook = [[FastAddressBook alloc] init];
	}

	linphone_core_set_zrtp_secrets_file(theLinphoneCore, [zrtpSecretsFileName UTF8String]);
	//linphone_core_set_chat_database_path(theLinphoneCore, [chatDBFileName UTF8String]);
	linphone_core_set_call_logs_database_path(theLinphoneCore, [chatDBFileName UTF8String]);

	[self setupNetworkReachabilityCallback];

	NSString *path = [LinphoneManager bundleFile:@"nowebcamCIF.jpg"];
	if (path) {
		const char *imagePath = [path UTF8String];
		LOGI(@"Using '%s' as source image for no webcam", imagePath);
		linphone_core_set_static_picture(theLinphoneCore, imagePath);
	}

	/*DETECT cameras*/
	_frontCamId = _backCamId = nil;
	char **camlist = (char **)linphone_core_get_video_devices(theLinphoneCore);
	if (camlist) {
		for (char *cam = *camlist; *camlist != NULL; cam = *++camlist) {
			if (strcmp(FRONT_CAM_NAME, cam) == 0) {
				_frontCamId = cam;
				// great set default cam to front
				LOGI(@"Setting default camera [%s]", _frontCamId);
				linphone_core_set_video_device(theLinphoneCore, _frontCamId);
			}
			if (strcmp(BACK_CAM_NAME, cam) == 0) {
				_backCamId = cam;
			}
		}
	} else {
		LOGW(@"No camera detected!");
	}

	if (![LinphoneManager isNotIphone3G]) {
		PayloadType *pt = linphone_core_find_payload_type(theLinphoneCore, "SILK", 24000, -1);
		if (pt) {
			linphone_core_enable_payload_type(theLinphoneCore, pt, FALSE);
			LOGW(@"SILK/24000 and video disabled on old iPhone 3G");
		}
		linphone_core_enable_video_display(theLinphoneCore, FALSE);
		linphone_core_enable_video_capture(theLinphoneCore, FALSE);
	}

	[self enableProxyPublish:([UIApplication sharedApplication].applicationState == UIApplicationStateActive)];

    //update UserDefaults for widgets
    [HistoryListTableView saveDataToUserDefaults];

	LOGI(@"Linphone [%s] started on [%s]", linphone_core_get_version(), [[UIDevice currentDevice].model UTF8String]);

	// Post event
	NSDictionary *dict = [NSDictionary dictionaryWithObject:[NSValue valueWithPointer:theLinphoneCore] forKey:@"core"];

	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCoreUpdate
													  object:LinphoneManager.instance
													userInfo:dict];
}

static BOOL libStarted = FALSE;

- (void)startLinphoneCore {

	if (libStarted) {
		LOGE(@"Liblinphone is already initialized!");
		return;
	}

	libStarted = TRUE;

	connectivity = none;
	signal(SIGPIPE, SIG_IGN);

	// create linphone core
	[self createLinphoneCore];
	[self.providerDelegate config];
	_iapManager = [[InAppProductsManager alloc] init];

	// - Security fix - remove multi transport migration, because it enables tcp or udp, if by factoring settings only
	// tls is enabled. 	This is a problem for new installations.
	// linphone_core_migrate_to_multi_transport(theLinphoneCore);

	// init audio session (just getting the instance will init)
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];
	BOOL bAudioInputAvailable = audioSession.inputAvailable;
	NSError *err = nil;

	if (![audioSession setActive:NO error:&err] && err) {
		LOGE(@"audioSession setActive failed: %@", [err description]);
		err = nil;
	}
	if (!bAudioInputAvailable) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"No microphone", nil)
																		 message:NSLocalizedString(@"You need to plug a microphone to your device to use the application.", nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
	}

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
		// go directly to bg mode
		[self enterBackgroundMode];
	}
}

void popup_link_account_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	if (status == LinphoneAccountCreatorStatusAccountLinked) {
		[LinphoneManager.instance lpConfigSetInt:0 forKey:@"must_link_account_time"];
	} else {
		LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
		if (cfg &&
			strcmp(linphone_proxy_config_get_domain(cfg),
				   [LinphoneManager.instance lpConfigStringForKey:@"domain_name"
														inSection:@"app"
													  withDefault:@"sip.linphone.org"]
					   .UTF8String) == 0) {
			UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Link your account", nil)
																			 message:[NSString stringWithFormat:NSLocalizedString(@"Link your Linphone.org account %s to your phone number.", nil),
																					  linphone_address_get_username(linphone_proxy_config_get_identity_address(cfg))]
																	  preferredStyle:UIAlertControllerStyleAlert];

			UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Maybe later", nil)
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];

			UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Let's go", nil)
																	 style:UIAlertActionStyleDefault
																   handler:^(UIAlertAction * action) {
																	   [PhoneMainView.instance changeCurrentView:AssistantLinkView.compositeViewDescription];
																   }];
			defaultAction.accessibilityLabel = @"Later";
			[errView addAction:defaultAction];
			[errView addAction:continueAction];
			[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];

			[LinphoneManager.instance
				lpConfigSetInt:[[NSDate date] dateByAddingTimeInterval:[LinphoneManager.instance
																		   lpConfigIntForKey:@"link_account_popup_time"
																				 withDefault:84200]]
								   .timeIntervalSince1970
						forKey:@"must_link_account_time"];
		}
	}
}

- (void)shouldPresentLinkPopup {
	NSDate *nextTime =
		[NSDate dateWithTimeIntervalSince1970:[self lpConfigIntForKey:@"must_link_account_time" withDefault:1]];
	NSDate *now = [NSDate date];
	if (nextTime.timeIntervalSince1970 > 0 && [now earlierDate:nextTime] == nextTime) {
		LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
		if (cfg) {
			const char *username = linphone_address_get_username(linphone_proxy_config_get_identity_address(cfg));
			LinphoneAccountCreator *account_creator = linphone_account_creator_new(
				LC,
				[LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
					.UTF8String);
			linphone_account_creator_set_user_data(account_creator, (__bridge void *)(self));
			linphone_account_creator_cbs_set_is_account_linked(linphone_account_creator_get_callbacks(account_creator),
															   popup_link_account_cb);
			linphone_account_creator_set_username(account_creator, username);
			linphone_account_creator_is_account_linked(account_creator);
		}
	}
}

- (void)createLinphoneCore {
	[self migrationAllPre];
	if (theLinphoneCore != nil) {
		LOGI(@"linphonecore is already created");
		return;
	}

	connectivity = none;

	// Set audio assets
	NSString *ring =
		([LinphoneManager bundleFile:[self lpConfigStringForKey:@"local_ring" inSection:@"sound"].lastPathComponent]
			 ?: [LinphoneManager bundleFile:@"notes_of_the_optimistic.caf"])
			.lastPathComponent;
	NSString *ringback =
		([LinphoneManager bundleFile:[self lpConfigStringForKey:@"remote_ring" inSection:@"sound"].lastPathComponent]
			 ?: [LinphoneManager bundleFile:@"ringback.wav"])
			.lastPathComponent;
	NSString *hold =
		([LinphoneManager bundleFile:[self lpConfigStringForKey:@"hold_music" inSection:@"sound"].lastPathComponent]
			 ?: [LinphoneManager bundleFile:@"hold.mkv"])
			.lastPathComponent;
	[self lpConfigSetString:[LinphoneManager bundleFile:ring] forKey:@"local_ring" inSection:@"sound"];
	[self lpConfigSetString:[LinphoneManager bundleFile:ringback] forKey:@"remote_ring" inSection:@"sound"];
	[self lpConfigSetString:[LinphoneManager bundleFile:hold] forKey:@"hold_music" inSection:@"sound"];

	LinphoneFactory *factory = linphone_factory_get();
	LinphoneCoreCbs *cbs = linphone_factory_create_core_cbs(factory);
	linphone_core_cbs_set_call_state_changed(cbs, linphone_iphone_call_state);
	linphone_core_cbs_set_registration_state_changed(cbs,linphone_iphone_registration_state);
	linphone_core_cbs_set_notify_presence_received_for_uri_or_tel(cbs, linphone_iphone_notify_presence_received_for_uri_or_tel);
	linphone_core_cbs_set_authentication_requested(cbs, linphone_iphone_popup_password_request);
	linphone_core_cbs_set_message_received(cbs, linphone_iphone_message_received);
	linphone_core_cbs_set_message_received_unable_decrypt(cbs, linphone_iphone_message_received_unable_decrypt);
	linphone_core_cbs_set_transfer_state_changed(cbs, linphone_iphone_transfer_state_changed);
	linphone_core_cbs_set_is_composing_received(cbs, linphone_iphone_is_composing_received);
	linphone_core_cbs_set_configuring_status(cbs, linphone_iphone_configuring_status_changed);
	linphone_core_cbs_set_global_state_changed(cbs, linphone_iphone_global_state_changed);
	linphone_core_cbs_set_notify_received(cbs, linphone_iphone_notify_received);
	linphone_core_cbs_set_call_encryption_changed(cbs, linphone_iphone_call_encryption_changed);
	linphone_core_cbs_set_chat_room_state_changed(cbs, linphone_iphone_chatroom_state_changed);
	linphone_core_cbs_set_version_update_check_result_received(cbs, linphone_iphone_version_update_check_result_received);
	linphone_core_cbs_set_qrcode_found(cbs, linphone_iphone_qr_code_found);
	linphone_core_cbs_set_user_data(cbs, (__bridge void *)(self));

	theLinphoneCore = linphone_factory_create_core_with_config_3(factory, _configDb, NULL);
	linphone_core_add_callbacks(theLinphoneCore, cbs);
	linphone_core_start(theLinphoneCore);

	// Let the core handle cbs
	linphone_core_cbs_unref(cbs);

	LOGI(@"Create linphonecore %p", theLinphoneCore);

	// Load plugins if available in the linphone SDK - otherwise these calls will do nothing
	MSFactory *f = linphone_core_get_ms_factory(theLinphoneCore);
	libmssilk_init(f);
	libmsamr_init(f);
	libmsx264_init(f);
	libmsopenh264_init(f);
	libmswebrtc_init(f);
	libmscodec2_init(f);

	linphone_core_reload_ms_plugins(theLinphoneCore, NULL);
	[self migrationAllPost];

	/* Use the rootca from framework, which is already set*/
	//linphone_core_set_root_ca(theLinphoneCore, [LinphoneManager bundleFile:@"rootca.pem"].UTF8String);
	linphone_core_set_user_certificates_path(theLinphoneCore, [LinphoneManager cacheDirectory].UTF8String);

	/* The core will call the linphone_iphone_configuring_status_changed callback when the remote provisioning is loaded
	 (or skipped).
	 Wait for this to finish the code configuration */

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(audioSessionInterrupted:)
											   name:AVAudioSessionInterruptionNotification
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(globalStateChangedNotificationHandler:)
											   name:kLinphoneGlobalStateUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(configuringStateChangedNotificationHandler:)
											   name:kLinphoneConfiguringStateUpdate
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(inappReady:) name:kIAPReady object:nil];

	/*call iterate once immediately in order to initiate background connections with sip server or remote provisioning
	 * grab, if any */
	[self iterate];
	// start scheduler
	mIterateTimer =
		[NSTimer scheduledTimerWithTimeInterval:0.02 target:self selector:@selector(iterate) userInfo:nil repeats:YES];
}

- (void)destroyLinphoneCore {
	[mIterateTimer invalidate];
	// just in case
	[self removeCTCallCenterCb];

	if (theLinphoneCore != nil) { // just in case application terminate before linphone core initialization

		for (FileTransferDelegate *ftd in _fileTransferDelegates) {
			[ftd stopAndDestroy];
		}
		[_fileTransferDelegates removeAllObjects];

		linphone_core_destroy(theLinphoneCore);
		LOGI(@"Destroy linphonecore %p", theLinphoneCore);
		theLinphoneCore = nil;

		// Post event
		NSDictionary *dict =
			[NSDictionary dictionaryWithObject:[NSValue valueWithPointer:theLinphoneCore] forKey:@"core"];
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCoreUpdate
														  object:LinphoneManager.instance
														userInfo:dict];

		SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		if (proxyReachability)
			CFRelease(proxyReachability);
		proxyReachability = nil;
	}
	libStarted = FALSE;
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)resetLinphoneCore {
	[self destroyLinphoneCore];
	[self createLinphoneCore];
	// reload friends
        [self.fastAddressBook fetchContactsInBackGroundThread];

        // reset network state to trigger a new network connectivity assessment
        linphone_core_set_network_reachable(theLinphoneCore, FALSE);
}

static int comp_call_id(const LinphoneCall *call, const char *callid) {
	if (linphone_call_log_get_call_id(linphone_call_get_call_log(call)) == nil) {
		ms_error("no callid for call [%p]", call);
		return 1;
	}
	return strcmp(linphone_call_log_get_call_id(linphone_call_get_call_log(call)), callid);
}

- (LinphoneCall *)callByCallId:(NSString *)call_id {
	const bctbx_list_t *calls = linphone_core_get_calls(theLinphoneCore);
	if (!calls || !call_id) {
		return NULL;
	}
	bctbx_list_t *call_tmp = bctbx_list_find_custom(calls, (bctbx_compare_func)comp_call_id, [call_id UTF8String]);
	if (!call_tmp) {
		return NULL;
	}
	LinphoneCall *call = (LinphoneCall *)call_tmp->data;
	return call;
}

- (void)cancelLocalNotifTimerForCallId:(NSString *)callid {
	// first, make sure this callid is not already involved in a call
	const bctbx_list_t *calls = linphone_core_get_calls(theLinphoneCore);
	bctbx_list_t *call = bctbx_list_find_custom(calls, (bctbx_compare_func)comp_call_id, [callid UTF8String]);
	if (call != NULL) {
		LinphoneCallAppData *data =
			(__bridge LinphoneCallAppData *)(linphone_call_get_user_data((LinphoneCall *)call->data));
		if (data->timer)
			[data->timer invalidate];
		data->timer = nil;
		return;
	}
}

- (void)acceptCallForCallId:(NSString *)callid {
	// first, make sure this callid is not already involved in a call
	const bctbx_list_t *calls = linphone_core_get_calls(theLinphoneCore);
	bctbx_list_t *call = bctbx_list_find_custom(calls, (bctbx_compare_func)comp_call_id, [callid UTF8String]);
	if (call != NULL) {
		const LinphoneVideoPolicy *video_policy = linphone_core_get_video_policy(theLinphoneCore);
		bool with_video = video_policy->automatically_accept;
		[self acceptCall:(LinphoneCall *)call->data evenWithVideo:with_video];
		return;
	};
}

- (void)addPushCallId:(NSString *)callid {
	// first, make sure this callid is not already involved in a call
	const bctbx_list_t *calls = linphone_core_get_calls(theLinphoneCore);
	if (bctbx_list_find_custom(calls, (bctbx_compare_func)comp_call_id, [callid UTF8String])) {
		LOGW(@"Call id [%@] already handled", callid);
		return;
	};
	if ([pushCallIDs count] > 10 /*max number of pending notif*/)
		[pushCallIDs removeObjectAtIndex:0];

	[pushCallIDs addObject:callid];
}

- (BOOL)popPushCallID:(NSString *)callId {
	for (NSString *pendingNotif in pushCallIDs) {
		if ([pendingNotif compare:callId] == NSOrderedSame) {
			[pushCallIDs removeObject:pendingNotif];
			return TRUE;
		}
	}
	return FALSE;
}

- (BOOL)resignActive {
	linphone_core_stop_dtmf_stream(theLinphoneCore);

	return YES;
}

- (void)playMessageSound {
	BOOL success = [self.messagePlayer play];
	if (!success) {
		LOGE(@"Could not play the message sound");
	}
	AudioServicesPlaySystemSound(LinphoneManager.instance.sounds.vibrate);
}

static int comp_call_state_paused(const LinphoneCall *call, const void *param) {
	return linphone_call_get_state(call) != LinphoneCallPaused;
}

- (void)startCallPausedLongRunningTask {
	pausedCallBgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
	  LOGW(@"Call cannot be paused any more, too late");
	  [[UIApplication sharedApplication] endBackgroundTask:pausedCallBgTask];
	}];
	LOGI(@"Long running task started, remaining [%@] because at least one call is paused",
		 [LinphoneUtils intervalToString:[[UIApplication sharedApplication] backgroundTimeRemaining]]);
}

- (void)startPushLongRunningTask:(NSString *)loc_key callId:(NSString *)callId {
	if (!callId)
		return;

	if ([callId isEqualToString:@""])
		return;

	if ([loc_key isEqualToString:@"IM_MSG"]) {
		[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskMsg];
		pushBgTaskMsg = 0;
		pushBgTaskMsg = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
		  if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive) {
			  LOGW(@"Incomming message with call-id [%@] couldn't be received", callId);
			  UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
			  content.title = NSLocalizedString(@"Message received", nil);
			  content.body = NSLocalizedString(@"You have received a message.", nil);
			  content.categoryIdentifier = @"push_msg";

			  UNNotificationRequest *req =
				  [UNNotificationRequest requestWithIdentifier:@"push_msg" content:content trigger:NULL];
			  [[UNUserNotificationCenter currentNotificationCenter]
				  addNotificationRequest:req
				   withCompletionHandler:^(NSError *_Nullable error) {
					 // Enable or disable features based on authorization.
					 if (error) {
						 LOGD(@"Error while adding notification request :");
						 LOGD(error.description);
					 }
				   }];
		  }
		  for (NSString *key in [LinphoneManager.instance.pushDict allKeys]) {
			  [LinphoneManager.instance.pushDict setValue:[NSNumber numberWithInt:0] forKey:key];
		  }
		  [[UIApplication sharedApplication] endBackgroundTask:pushBgTaskMsg];
		  pushBgTaskMsg = 0;
		}];
		LOGI(@"Message long running task started for call-id [%@], remaining [%@] because a push has been received",
			 callId, [LinphoneUtils intervalToString:[[UIApplication sharedApplication] backgroundTimeRemaining]]);
	} else if ([loc_key isEqualToString:@"IC_MSG"]) {
		[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskCall];
		pushBgTaskCall = 0;
		pushBgTaskCall = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
			//does not make sens to notify user as we have no information on this missed called
			for (NSString *key in [LinphoneManager.instance.pushDict allKeys]) {
				[LinphoneManager.instance.pushDict setValue:[NSNumber numberWithInt:0] forKey:key];
			}
			[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskCall];
			pushBgTaskCall = 0;
		}];
		LOGI(@"Call long running task started for call-id [%@], remaining [%@] because a push has been received",
			 callId, [LinphoneUtils intervalToString:[[UIApplication sharedApplication] backgroundTimeRemaining]]);
	} else if ([loc_key isEqualToString:@"IC_SIL"]) {
		[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskRefer];
		pushBgTaskRefer = 0;
		pushBgTaskRefer = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
			// Could be or not an error since the app doesn't know when to end the background task for a REFER
			// TODO: Manage pushes in the SDK
			if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive)
				LOGI(@"Incomming refer long running task with call-id [%@] has expired", callId);

			for (NSString *key in [LinphoneManager.instance.pushDict allKeys]) {
				[LinphoneManager.instance.pushDict setValue:[NSNumber numberWithInt:0] forKey:key];
			}
			[[UIApplication sharedApplication] endBackgroundTask:pushBgTaskRefer];
			pushBgTaskRefer = 0;
		}];
		LOGI(@"Refer long running task started for call-id [%@], remaining [%@] because a push has been received",
			 callId, [LinphoneUtils intervalToString:[[UIApplication sharedApplication] backgroundTimeRemaining]]);
	}
}

- (void)enableProxyPublish:(BOOL)enabled {
	if (linphone_core_get_global_state(LC) != LinphoneGlobalOn || !linphone_core_get_default_friend_list(LC)) {
		LOGW(@"Not changing presence configuration because linphone core not ready yet");
		return;
	}

	if ([self lpConfigBoolForKey:@"publish_presence"]) {
		// set present to "tv", because "available" does not work yet
		if (enabled) {
			linphone_core_set_presence_model(LC, linphone_core_create_presence_model_with_activity(LC, LinphonePresenceActivityTV, NULL));
		}

		const MSList *proxies = linphone_core_get_proxy_config_list(LC);
		while (proxies) {
			LinphoneProxyConfig *cfg = proxies->data;
			linphone_proxy_config_edit(cfg);
			linphone_proxy_config_enable_publish(cfg, enabled);
			linphone_proxy_config_done(cfg);
			proxies = proxies->next;
		}
		// force registration update first, then update friend list subscription
		[self iterate];
	}

	linphone_core_enable_friend_list_subscription(LC, enabled && [LinphoneManager.instance lpConfigBoolForKey:@"use_rls_presence"]);
}

- (BOOL)enterBackgroundMode {
	linphone_core_enter_background(LC);

	LinphoneProxyConfig *proxyCfg = linphone_core_get_default_proxy_config(theLinphoneCore);
	BOOL shouldEnterBgMode = FALSE;

	// disable presence
	[self enableProxyPublish:NO];

	// handle proxy config if any
	if (proxyCfg) {
		const char *refkey = proxyCfg ? linphone_proxy_config_get_ref_key(proxyCfg) : NULL;
		BOOL pushNotifEnabled = (refkey && strcmp(refkey, "push_notification") == 0);
		if ([LinphoneManager.instance lpConfigBoolForKey:@"backgroundmode_preference"] || pushNotifEnabled) {
			if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
				// For registration register
				[self refreshRegisters];
			}
		}

		if ([LinphoneManager.instance lpConfigBoolForKey:@"voip_mode_preference"] && [LinphoneManager.instance lpConfigBoolForKey:@"backgroundmode_preference"] && !pushNotifEnabled) {
            // Keep this!! Socket VoIP is deprecated after 9.0, but sometimes it's the only way to keep the phone background and receive the call. For example, when there is only local area network.
            // register keepalive
            if ([[UIApplication sharedApplication]
                 setKeepAliveTimeout:600 /*(NSTimeInterval)linphone_proxy_config_get_expires(proxyCfg)*/
                 handler:^{
                     LOGW(@"keepalive handler");
                     mLastKeepAliveDate = [NSDate date];
                     if (theLinphoneCore == nil) {
                         LOGW(@"It seems that Linphone BG mode was deactivated, just skipping");
                         return;
                     }
                     [_iapManager check];
                     if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
                         // For registration register
                         [self refreshRegisters];
                     }
                     linphone_core_iterate(theLinphoneCore);
                 }]) {
                     
                     LOGI(@"keepalive handler succesfully registered");
                 } else {
                     LOGI(@"keepalive handler cannot be registered");
                 }
			shouldEnterBgMode = TRUE;
		}
	}

	LinphoneCall *currentCall = linphone_core_get_current_call(theLinphoneCore);
	const bctbx_list_t *callList = linphone_core_get_calls(theLinphoneCore);
	if (!currentCall // no active call
		&& callList  // at least one call in a non active state
		&& bctbx_list_find_custom(callList, (bctbx_compare_func)comp_call_state_paused, NULL)) {
		[self startCallPausedLongRunningTask];
	}
	if (callList) // If at least one call exist, enter normal bg mode
		shouldEnterBgMode = TRUE;

	// Stop the video preview
	if (theLinphoneCore) {
		linphone_core_enable_video_preview(theLinphoneCore, FALSE);
		[self iterate];
	}
	linphone_core_stop_dtmf_stream(theLinphoneCore);

	LOGI(@"Entering [%s] bg mode", shouldEnterBgMode ? "normal" : "lite");
	if (!shouldEnterBgMode && floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
		const char *refkey = proxyCfg ? linphone_proxy_config_get_ref_key(proxyCfg) : NULL;
		BOOL pushNotifEnabled = (refkey && strcmp(refkey, "push_notification") == 0);
		if (pushNotifEnabled) {
			LOGI(@"Keeping lc core to handle push");
			// Destroy voip socket if any and reset connectivity mode
			connectivity = none;
			linphone_core_set_network_reachable(theLinphoneCore, FALSE);
			return YES;
		}
		return NO;
	}
	return YES;
}

- (void)becomeActive {
	linphone_core_enter_foreground(LC);

    [self checkNewVersion];

	// enable presence
	if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max || self.connectivity == none) {
		[self refreshRegisters];
	}
	if (pausedCallBgTask) {
		[[UIApplication sharedApplication] endBackgroundTask:pausedCallBgTask];
		pausedCallBgTask = 0;
	}
	if (incallBgTask) {
		[[UIApplication sharedApplication] endBackgroundTask:incallBgTask];
		incallBgTask = 0;
	}

	/*IOS specific*/
	linphone_core_start_dtmf_stream(theLinphoneCore);
	[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
							 completionHandler:^(BOOL granted){
							 }];

	/*start the video preview in case we are in the main view*/
	if (linphone_core_video_display_enabled(theLinphoneCore) && [self lpConfigBoolForKey:@"preview_preference"]) {
		linphone_core_enable_video_preview(theLinphoneCore, TRUE);
	}
	/*check last keepalive handler date*/
	if (mLastKeepAliveDate != Nil) {
		NSDate *current = [NSDate date];
		if ([current timeIntervalSinceDate:mLastKeepAliveDate] > 700) {
			NSString *datestr = [mLastKeepAliveDate description];
			LOGW(@"keepalive handler was called for the last time at %@", datestr);
		}
	}

	[self enableProxyPublish:YES];
}

- (void)beginInterruption {
	LinphoneCall *c = linphone_core_get_current_call(theLinphoneCore);
	LOGI(@"Sound interruption detected!");
	if (c && linphone_call_get_state(c) == LinphoneCallStreamsRunning) {
		_speakerBeforePause = _speakerEnabled;
		linphone_call_pause(c);
	}
}

- (void)endInterruption {
	LOGI(@"Sound interruption ended!");
}

- (void)refreshRegisters {
	if (connectivity == none) {
		// don't trust ios when he says there is no network. Create a new reachability context, the previous one might
		// be mis-functionning.
        LOGI(@"None connectivity");
		[self setupNetworkReachabilityCallback];
	}
    LOGI(@"Network reachability callback setup");
    linphone_core_refresh_registers(theLinphoneCore); // just to make sure REGISTRATION is up to date
}

- (void)migrateImportantFiles {
    if ([LinphoneManager copyFile:[LinphoneManager documentFile:@"linphonerc"] destination:[LinphoneManager preferenceFile:@"linphonerc"] override:TRUE])
        [NSFileManager.defaultManager
         removeItemAtPath:[LinphoneManager documentFile:@"linphonerc"]
         error:nil];
    
    if ([LinphoneManager copyFile:[LinphoneManager documentFile:@"linphone_chats.db"] destination:[LinphoneManager dataFile:@"linphone_chats.db"] override:TRUE])
        [NSFileManager.defaultManager
         removeItemAtPath:[LinphoneManager documentFile:@"linphone_chats.db"]
         error:nil];
    
    if ([LinphoneManager copyFile:[LinphoneManager documentFile:@"zrtp_secrets"] destination:[LinphoneManager dataFile:@"zrtp_secrets"] override:TRUE])
        [NSFileManager.defaultManager
         removeItemAtPath:[LinphoneManager documentFile:@"zrtp_secrets"]
         error:nil];
    
    if ([LinphoneManager copyFile:[LinphoneManager documentFile:@"zrtp_secrets.bkp"] destination:[LinphoneManager dataFile:@"zrtp_secrets.bkp"] override:TRUE])
        [NSFileManager.defaultManager
         removeItemAtPath:[LinphoneManager documentFile:@"zrtp_secrets.bkp"]
         error:nil];
}

- (void)renameDefaultSettings {
	// rename .linphonerc to linphonerc to ease debugging: when downloading
	// containers from MacOSX, Finder do not display hidden files leading
	// to useless painful operations to display the .linphonerc file
	NSString *src = [LinphoneManager documentFile:@".linphonerc"];
	NSString *dst = [LinphoneManager preferenceFile:@"linphonerc"];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSError *fileError = nil;
	if ([fileManager fileExistsAtPath:src]) {
		if ([fileManager fileExistsAtPath:dst]) {
			[fileManager removeItemAtPath:src error:&fileError];
			LOGW(@"%@ already exists, simply removing %@ %@", dst, src,
				 fileError ? fileError.localizedDescription : @"successfully");
		} else {
			[fileManager moveItemAtPath:src toPath:dst error:&fileError];
			LOGI(@"%@ moving to %@ %@", dst, src, fileError ? fileError.localizedDescription : @"successfully");
		}
	}
}

- (void)copyDefaultSettings {
	NSString *src = [LinphoneManager bundleFile:@"linphonerc"];
	NSString *srcIpad = [LinphoneManager bundleFile:@"linphonerc~ipad"];
	if (IPAD && [[NSFileManager defaultManager] fileExistsAtPath:srcIpad]) {
		src = srcIpad;
	}
	NSString *dst = [LinphoneManager preferenceFile:@"linphonerc"];
	[LinphoneManager copyFile:src destination:dst override:FALSE];
}

- (void)overrideDefaultSettings {
	NSString *factory = [LinphoneManager bundleFile:@"linphonerc-factory"];
	NSString *factoryIpad = [LinphoneManager bundleFile:@"linphonerc-factory~ipad"];
	if (IPAD && [[NSFileManager defaultManager] fileExistsAtPath:factoryIpad]) {
		factory = factoryIpad;
	}
	NSString *confiFileName = [LinphoneManager preferenceFile:@"linphonerc"];
	_configDb = lp_config_new_with_factory([confiFileName UTF8String], [factory UTF8String]);
}
#pragma mark - Audio route Functions

- (bool)allowSpeaker {
	if (IPAD)
		return true;

	bool allow = true;
	AVAudioSessionRouteDescription *newRoute = [AVAudioSession sharedInstance].currentRoute;
	if (newRoute && newRoute.outputs.count > 0) {
		NSString *route = newRoute.outputs[0].portType;
		allow = !([route isEqualToString:AVAudioSessionPortLineOut] ||
				  [route isEqualToString:AVAudioSessionPortHeadphones] ||
				  [[AudioHelper bluetoothRoutes] containsObject:route]);
	}
	return allow;
}

- (void)audioRouteChangeListenerCallback:(NSNotification *)notif {
	if (IPAD)
		return;

	// there is at least one bug when you disconnect an audio bluetooth headset
	// since we only get notification of route having changed, we cannot tell if that is due to:
	// -bluetooth headset disconnected or
	// -user wanted to use earpiece
	// the only thing we can assume is that when we lost a device, it must be a bluetooth one (strong hypothesis though)
	if ([[notif.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] integerValue] == AVAudioSessionRouteChangeReasonOldDeviceUnavailable)
		_bluetoothAvailable = NO;

	AVAudioSessionRouteDescription *newRoute = [AVAudioSession sharedInstance].currentRoute;

	if (newRoute && newRoute.outputs.count > 0) {
		NSString *route = newRoute.outputs[0].portType;
		LOGI(@"Current audio route is [%s]", [route UTF8String]);

		_speakerEnabled = [route isEqualToString:AVAudioSessionPortBuiltInSpeaker];
		if (([[AudioHelper bluetoothRoutes] containsObject:route]) && !_speakerEnabled) {
			_bluetoothAvailable = TRUE;
			_bluetoothEnabled = TRUE;
		} else
			_bluetoothEnabled = FALSE;

		NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:_bluetoothAvailable], @"available", nil];
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneBluetoothAvailabilityUpdate
														  object:self
														userInfo:dict];
	}
}

- (void)setSpeakerEnabled:(BOOL)enable {
	_speakerEnabled = enable;
	NSError *err = nil;

	if (enable && [self allowSpeaker]) {
		[[AVAudioSession sharedInstance] overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:&err];
		[[UIDevice currentDevice] setProximityMonitoringEnabled:FALSE];
		_bluetoothEnabled = FALSE;
	} else {
		AVAudioSessionPortDescription *builtinPort = [AudioHelper builtinAudioDevice];
		[[AVAudioSession sharedInstance] setPreferredInput:builtinPort error:&err];
		[[UIDevice currentDevice] setProximityMonitoringEnabled:(linphone_core_get_calls_nb(LC) > 0)];
	}

	if (err) {
		LOGE(@"Failed to change audio route: err %@", err.localizedDescription);
		err = nil;
	}
}

- (void)setBluetoothEnabled:(BOOL)enable {
	if (_bluetoothAvailable) {
		// The change of route will be done in setSpeakerEnabled
		_bluetoothEnabled = enable;
		if (_bluetoothEnabled) {
			NSError *err = nil;
			AVAudioSessionPortDescription *_bluetoothPort = [AudioHelper bluetoothAudioDevice];
			[[AVAudioSession sharedInstance] setPreferredInput:_bluetoothPort error:&err];
			// if setting bluetooth failed, it must be because the device is not available
			// anymore (disconnected), so deactivate bluetooth.
			if (err) {
				_bluetoothEnabled = FALSE;
				LOGE(@"Failed to enable bluetooth: err %@", err.localizedDescription);
				err = nil;
			} else {
				_speakerEnabled = FALSE;
				return;
			}
		}
	}
	[self setSpeakerEnabled:_speakerEnabled];
}

#pragma mark - Call Functions

- (void)acceptCall:(LinphoneCall *)call evenWithVideo:(BOOL)video {
	LinphoneCallParams *lcallParams = linphone_core_create_call_params(theLinphoneCore, call);
	if (!lcallParams) {
		LOGW(@"Could not create call parameters for %p, call has probably already ended.", call);
		return;
	}

	if ([self lpConfigBoolForKey:@"edge_opt_preference"]) {
		bool low_bandwidth = self.network == network_2g;
		if (low_bandwidth) {
			LOGI(@"Low bandwidth mode");
		}
		linphone_call_params_enable_low_bandwidth(lcallParams, low_bandwidth);
	}
	linphone_call_params_enable_video(lcallParams, video);

    //We set the record file name here because we can't do it after the call is started.
    NSString *writablePath = [LinphoneUtils recordingFilePathFromCall:linphone_call_log_get_from_address(linphone_call_get_call_log(call))];
    LOGD(@"record file path: %@\n", writablePath);
    
    linphone_call_params_set_record_file(lcallParams, [writablePath cStringUsingEncoding:NSUTF8StringEncoding]);
    
	linphone_call_accept_with_params(call, lcallParams);
	linphone_call_params_unref(lcallParams);
}

- (void)send:(NSString *)replyText toChatRoom:(LinphoneChatRoom *)room {
	LinphoneChatMessage *msg = linphone_chat_room_create_message(room, replyText.UTF8String);
	linphone_chat_room_send_chat_message(room, msg);

	if (linphone_core_lime_enabled(LC) == LinphoneLimeMandatory && !linphone_chat_room_lime_available(room))
		[LinphoneManager.instance alertLIME:room];

	[ChatConversationView markAsRead:room];
}

- (void)call:(const LinphoneAddress *)iaddr {
	// First verify that network is available, abort otherwise.
	if (!linphone_core_is_network_reachable(theLinphoneCore)) {
		[PhoneMainView.instance presentViewController:[LinphoneUtils networkErrorView] animated:YES completion:nil];
		return;
	}

	// Then check that no GSM calls are in progress, abort otherwise.
	CTCallCenter *callCenter = [[CTCallCenter alloc] init];
	if ([callCenter currentCalls] != nil && floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
		LOGE(@"GSM call in progress, cancelling outgoing SIP call request");
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Cannot make call", nil)
																		 message:NSLocalizedString(@"Please terminate GSM call first.", nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
		return;
	}

	// Then check that the supplied address is valid
	if (!iaddr) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Invalid SIP address", nil)
																		 message:NSLocalizedString(@"Either configure a SIP proxy server from settings prior to place a "
																								   @"call or use a valid SIP address (I.E sip:john@example.net)", nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];

		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
		return;
	}

	if (linphone_core_get_calls_nb(theLinphoneCore) < 1 &&
		floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max &&
		self.providerDelegate.callKitCalls < 1) {
		self.providerDelegate.callKitCalls++;
		NSUUID *uuid = [NSUUID UUID];
		[LinphoneManager.instance.providerDelegate.uuids setObject:uuid forKey:@""];
		[LinphoneManager.instance.providerDelegate.calls setObject:@"" forKey:uuid];
		LinphoneManager.instance.providerDelegate.pendingAddr = linphone_address_clone(iaddr);
		NSString *address = [FastAddressBook displayNameForAddress:iaddr];
		CXHandle *handle = [[CXHandle alloc] initWithType:CXHandleTypeGeneric value:address];
		CXStartCallAction *act = [[CXStartCallAction alloc] initWithCallUUID:uuid handle:handle];
		CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
		[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																	  completion:^(NSError *err){
																	  }];
	} else {
		[self doCall:iaddr];
	}
}

- (BOOL)doCall:(const LinphoneAddress *)iaddr {
    return [self doCallWithSas:iaddr isSas:false];
}

- (BOOL)doCallWithSas:(const LinphoneAddress *)iaddr isSas:(BOOL)isSas {
    LinphoneAddress *addr = linphone_address_clone(iaddr);
    NSString *displayName = [FastAddressBook displayNameForAddress:addr];
    
    // Finally we can make the call
    LinphoneCallParams *lcallParams = linphone_core_create_call_params(theLinphoneCore, NULL);
    if ([self lpConfigBoolForKey:@"edge_opt_preference"] && (self.network == network_2g)) {
        LOGI(@"Enabling low bandwidth mode");
        linphone_call_params_enable_low_bandwidth(lcallParams, YES);
    }
    
    if (displayName != nil) {
        linphone_address_set_display_name(addr, displayName.UTF8String);
    }
    if ([LinphoneManager.instance lpConfigBoolForKey:@"override_domain_with_default_one"]) {
        linphone_address_set_domain(
                                    addr, [[LinphoneManager.instance lpConfigStringForKey:@"domain" inSection:@"assistant"] UTF8String]);
    }
    
    LinphoneCall *call;
    if (LinphoneManager.instance.nextCallIsTransfer) {
        char *caddr = linphone_address_as_string(addr);
        call = linphone_core_get_current_call(theLinphoneCore);
        linphone_call_transfer(call, caddr);
        LinphoneManager.instance.nextCallIsTransfer = NO;
        ms_free(caddr);
    } else {
        //We set the record file name here because we can't do it after the call is started.
        NSString *writablePath = [LinphoneUtils recordingFilePathFromCall:addr];
        LOGD(@"record file path: %@\n", writablePath);
        linphone_call_params_set_record_file(lcallParams, [writablePath cStringUsingEncoding:NSUTF8StringEncoding]);
        if (isSas)
            linphone_call_params_set_media_encryption(lcallParams, LinphoneMediaEncryptionZRTP);
        call = linphone_core_invite_address_with_params(theLinphoneCore, addr, lcallParams);
        if (call) {
            // The LinphoneCallAppData object should be set on call creation with callback
            // - (void)onCall:StateChanged:withMessage:. If not, we are in big trouble and expect it to crash
            // We are NOT responsible for creating the AppData.
            LinphoneCallAppData *data = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
            if (data == nil) {
                LOGE(@"New call instanciated but app data was not set. Expect it to crash.");
                /* will be used later to notify user if video was not activated because of the linphone core*/
            } else {
                data->videoRequested = linphone_call_params_video_enabled(lcallParams);
            }
        }
    }
    linphone_address_destroy(addr);
    linphone_call_params_destroy(lcallParams);
    
    return TRUE;
}

#pragma mark - Property Functions

- (void)setPushNotificationToken:(NSData *)apushNotificationToken {
	if (apushNotificationToken == _pushNotificationToken) {
		return;
	}
	_pushNotificationToken = apushNotificationToken;

	@try {
		const MSList *proxies = linphone_core_get_proxy_config_list(LC);
		while (proxies) {
			[self configurePushTokenForProxyConfig:proxies->data];
			proxies = proxies->next;
		}
	} @catch (NSException* e) {
		LOGW(@"%s: linphone core not ready yet, ignoring push token", __FUNCTION__);
	}
}

- (void)configurePushTokenForProxyConfig:(LinphoneProxyConfig *)proxyCfg {
	linphone_proxy_config_edit(proxyCfg);

	NSData *tokenData = _pushNotificationToken;
	const char *refkey = linphone_proxy_config_get_ref_key(proxyCfg);
	BOOL pushNotifEnabled = (refkey && strcmp(refkey, "push_notification") == 0);
	if (tokenData != nil && pushNotifEnabled) {
		const unsigned char *tokenBuffer = [tokenData bytes];
		NSMutableString *tokenString = [NSMutableString stringWithCapacity:[tokenData length] * 2];
		for (int i = 0; i < [tokenData length]; ++i) {
			[tokenString appendFormat:@"%02X", (unsigned int)tokenBuffer[i]];
		}
// NSLocalizedString(@"IC_MSG", nil); // Fake for genstrings
// NSLocalizedString(@"IM_MSG", nil); // Fake for genstrings
// NSLocalizedString(@"IM_FULLMSG", nil); // Fake for genstrings
#ifdef DEBUG
#define APPMODE_SUFFIX @"dev"
#else
#define APPMODE_SUFFIX @"prod"
#endif
		NSString *ring =
		([LinphoneManager bundleFile:[self lpConfigStringForKey:@"local_ring" inSection:@"sound"].lastPathComponent]
		 ?: [LinphoneManager bundleFile:@"notes_of_the_optimistic.caf"])
		.lastPathComponent;

		NSString *timeout;
		if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			timeout = @";pn-timeout=0";
		} else {
			timeout = @"";
		}

		NSString *params = [NSString
			stringWithFormat:@"app-id=%@.voip.%@;pn-type=apple;pn-tok=%@;pn-msg-str=IM_MSG;pn-call-str=IC_MSG;pn-"
							 @"call-snd=%@;pn-msg-snd=msg.caf%@;pn-silent=1",
							 [[NSBundle mainBundle] bundleIdentifier], APPMODE_SUFFIX, tokenString, ring, timeout];

		LOGI(@"Proxy config %s configured for push notifications with contact: %@",
			 linphone_proxy_config_get_identity(proxyCfg), params);
		linphone_proxy_config_set_contact_uri_parameters(proxyCfg, [params UTF8String]);
		linphone_proxy_config_set_contact_parameters(proxyCfg, NULL);
	} else {
		LOGI(@"Proxy config %s NOT configured for push notifications", linphone_proxy_config_get_identity(proxyCfg));
		// no push token:
		linphone_proxy_config_set_contact_uri_parameters(proxyCfg, NULL);
		linphone_proxy_config_set_contact_parameters(proxyCfg, NULL);
	}

	linphone_proxy_config_done(proxyCfg);
}

#pragma mark - Misc Functions
+ (PHFetchResult *)getPHAssets:(NSString *)key {
    PHFetchResult<PHAsset *> *assets;
    if ([key hasPrefix:@"assets-library"]) {
        // compability with previous linphone version
        assets = [PHAsset fetchAssetsWithALAssetURLs:@[[NSURL URLWithString:key]] options:nil];
    } else {
        assets = [PHAsset fetchAssetsWithLocalIdentifiers:[NSArray arrayWithObject:key] options:nil];
    }
    return assets;
}

+ (NSString *)bundleFile:(NSString *)file {
	return [[NSBundle mainBundle] pathForResource:[file stringByDeletingPathExtension] ofType:[file pathExtension]];
}

+ (NSString *)documentFile:(NSString *)file {
     NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
     NSString *documentsPath = [paths objectAtIndex:0];
     return [documentsPath stringByAppendingPathComponent:file];
}

+ (NSString *)preferenceFile:(NSString *)file {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *writablePath = [paths objectAtIndex:0];
    NSString *fullPath = [writablePath stringByAppendingString:@"/Preferences/linphone/"];
    if (![[NSFileManager defaultManager] fileExistsAtPath:fullPath]) {
        NSError *error;
        LOGI(@"Preference path %@ does not exist, creating it.",fullPath);
        if (![[NSFileManager defaultManager] createDirectoryAtPath:fullPath
                                       withIntermediateDirectories:YES
                                                        attributes:nil
                                                             error:&error]) {
            LOGE(@"Create preference path directory error: %@",error.description);
        }
    }
    
    return [fullPath stringByAppendingPathComponent:file];
}

+ (NSString *)dataFile:(NSString *)file {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *writablePath = [paths objectAtIndex:0];
    NSString *fullPath = [writablePath stringByAppendingString:@"/linphone/"];
    if (![[NSFileManager defaultManager] fileExistsAtPath:fullPath]) {
        NSError *error;
        LOGI(@"Data path %@ does not exist, creating it.",fullPath);
        if (![[NSFileManager defaultManager] createDirectoryAtPath:fullPath
                                       withIntermediateDirectories:YES
                                                        attributes:nil
                                                             error:&error]) {
            LOGE(@"Create data path directory error: %@",error.description);
        }
    }
    
    return [fullPath stringByAppendingPathComponent:file];
}

+ (NSString *)cacheDirectory {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString *cachePath = [paths objectAtIndex:0];
	BOOL isDir = NO;
	NSError *error;
	// cache directory must be created if not existing
	if (![[NSFileManager defaultManager] fileExistsAtPath:cachePath isDirectory:&isDir] && isDir == NO) {
		[[NSFileManager defaultManager] createDirectoryAtPath:cachePath
								  withIntermediateDirectories:NO
												   attributes:nil
														error:&error];
	}
	return cachePath;
}

+ (int)unreadMessageCount {
	int count = 0;
	const MSList *rooms = linphone_core_get_chat_rooms(LC);
	const MSList *item = rooms;
	while (item) {
		LinphoneChatRoom *room = (LinphoneChatRoom *)item->data;
		if (room) {
			count += linphone_chat_room_get_unread_messages_count(room);
		}
		item = item->next;
	}

	return count;
}

+ (BOOL)copyFile:(NSString *)src destination:(NSString *)dst override:(BOOL)override {
	NSFileManager *fileManager = NSFileManager.defaultManager;
	NSError *error = nil;
	if ([fileManager fileExistsAtPath:src] == NO) {
		LOGE(@"Can't find \"%@\": %@", src, [error localizedDescription]);
		return FALSE;
	}
	if ([fileManager fileExistsAtPath:dst] == YES) {
		if (override) {
			[fileManager removeItemAtPath:dst error:&error];
			if (error != nil) {
				LOGE(@"Can't remove \"%@\": %@", dst, [error localizedDescription]);
				return FALSE;
			}
		} else {
			LOGW(@"\"%@\" already exists", dst);
			return FALSE;
		}
	}
	[fileManager copyItemAtPath:src toPath:dst error:&error];
	if (error != nil) {
		LOGE(@"Can't copy \"%@\" to \"%@\": %@", src, dst, [error localizedDescription]);
		return FALSE;
	}
	return TRUE;
}

- (void)configureVbrCodecs {
	PayloadType *pt;
	int bitrate = lp_config_get_int(
		_configDb, "audio", "codec_bitrate_limit",
		kLinphoneAudioVbrCodecDefaultBitrate); /*default value is in linphonerc or linphonerc-factory*/
	const MSList *audio_codecs = linphone_core_get_audio_codecs(theLinphoneCore);
	const MSList *codec = audio_codecs;
	while (codec) {
		pt = codec->data;
		if (linphone_core_payload_type_is_vbr(theLinphoneCore, pt)) {
			linphone_core_set_payload_type_bitrate(theLinphoneCore, pt, bitrate);
		}
		codec = codec->next;
	}
}

+ (id)getMessageAppDataForKey:(NSString *)key inMessage:(LinphoneChatMessage *)msg {

	if (msg == nil)
		return nil;

	id value = nil;
	const char *appData = linphone_chat_message_get_appdata(msg);
	if (appData) {
		NSDictionary *appDataDict =
			[NSJSONSerialization JSONObjectWithData:[NSData dataWithBytes:appData length:strlen(appData)]
											options:0
											  error:nil];
		value = [appDataDict objectForKey:key];
	}
	return value;
}

+ (void)setValueInMessageAppData:(id)value forKey:(NSString *)key inMessage:(LinphoneChatMessage *)msg {
        NSMutableDictionary *appDataDict = [NSMutableDictionary dictionary];
        const char *appData = linphone_chat_message_get_appdata(msg);
        if (appData) {
            appDataDict = [NSJSONSerialization JSONObjectWithData:[NSData dataWithBytes:appData length:strlen(appData)]
                                                          options:NSJSONReadingMutableContainers
                                                            error:nil];
        }

        [appDataDict setValue:value forKey:key];

        NSData *data = [NSJSONSerialization dataWithJSONObject:appDataDict options:0 error:nil];
        NSString *appdataJSON = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        linphone_chat_message_set_appdata(msg, [appdataJSON UTF8String]);
}

#pragma mark - LPConfig Functions

- (void)lpConfigSetString:(NSString *)value forKey:(NSString *)key {
	[self lpConfigSetString:value forKey:key inSection:LINPHONERC_APPLICATION_KEY];
}
- (void)lpConfigSetString:(NSString *)value forKey:(NSString *)key inSection:(NSString *)section {
	if (!key)
		return;
	lp_config_set_string(_configDb, [section UTF8String], [key UTF8String], value ? [value UTF8String] : NULL);
}
- (NSString *)lpConfigStringForKey:(NSString *)key {
	return [self lpConfigStringForKey:key withDefault:nil];
}
- (NSString *)lpConfigStringForKey:(NSString *)key withDefault:(NSString *)defaultValue {
	return [self lpConfigStringForKey:key inSection:LINPHONERC_APPLICATION_KEY withDefault:defaultValue];
}
- (NSString *)lpConfigStringForKey:(NSString *)key inSection:(NSString *)section {
	return [self lpConfigStringForKey:key inSection:section withDefault:nil];
}
- (NSString *)lpConfigStringForKey:(NSString *)key inSection:(NSString *)section withDefault:(NSString *)defaultValue {
	if (!key)
		return defaultValue;
	const char *value = lp_config_get_string(_configDb, [section UTF8String], [key UTF8String], NULL);
	return value ? [NSString stringWithUTF8String:value] : defaultValue;
}

- (void)lpConfigSetInt:(int)value forKey:(NSString *)key {
	[self lpConfigSetInt:value forKey:key inSection:LINPHONERC_APPLICATION_KEY];
}
- (void)lpConfigSetInt:(int)value forKey:(NSString *)key inSection:(NSString *)section {
	if (!key)
		return;
	lp_config_set_int(_configDb, [section UTF8String], [key UTF8String], (int)value);
}
- (int)lpConfigIntForKey:(NSString *)key {
	return [self lpConfigIntForKey:key withDefault:-1];
}
- (int)lpConfigIntForKey:(NSString *)key withDefault:(int)defaultValue {
	return [self lpConfigIntForKey:key inSection:LINPHONERC_APPLICATION_KEY withDefault:defaultValue];
}
- (int)lpConfigIntForKey:(NSString *)key inSection:(NSString *)section {
	return [self lpConfigIntForKey:key inSection:section withDefault:-1];
}
- (int)lpConfigIntForKey:(NSString *)key inSection:(NSString *)section withDefault:(int)defaultValue {
	if (!key)
		return defaultValue;
	return lp_config_get_int(_configDb, [section UTF8String], [key UTF8String], (int)defaultValue);
}

- (void)lpConfigSetBool:(BOOL)value forKey:(NSString *)key {
	[self lpConfigSetBool:value forKey:key inSection:LINPHONERC_APPLICATION_KEY];
}
- (void)lpConfigSetBool:(BOOL)value forKey:(NSString *)key inSection:(NSString *)section {
	[self lpConfigSetInt:(int)(value == TRUE) forKey:key inSection:section];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key {
	return [self lpConfigBoolForKey:key withDefault:FALSE];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key withDefault:(BOOL)defaultValue {
	return [self lpConfigBoolForKey:key inSection:LINPHONERC_APPLICATION_KEY withDefault:defaultValue];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key inSection:(NSString *)section {
	return [self lpConfigBoolForKey:key inSection:section withDefault:FALSE];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key inSection:(NSString *)section withDefault:(BOOL)defaultValue {
	if (!key)
		return defaultValue;
	int val = [self lpConfigIntForKey:key inSection:section withDefault:-1];
	return (val != -1) ? (val == 1) : defaultValue;
}

#pragma mark - GSM management

- (void)removeCTCallCenterCb {
	if (mCallCenter != nil) {
		LOGI(@"Removing CT call center listener [%p]", mCallCenter);
		mCallCenter.callEventHandler = NULL;
	}
	mCallCenter = nil;
}

- (void)setupGSMInteraction {

	[self removeCTCallCenterCb];
	mCallCenter = [[CTCallCenter alloc] init];
	LOGI(@"Adding CT call center listener [%p]", mCallCenter);
	__block __weak LinphoneManager *weakSelf = self;
	__block __weak CTCallCenter *weakCCenter = mCallCenter;
	mCallCenter.callEventHandler = ^(CTCall *call) {
	  // post on main thread
	  [weakSelf performSelectorOnMainThread:@selector(handleGSMCallInteration:)
								 withObject:weakCCenter
							  waitUntilDone:YES];
	};
}

- (void)handleGSMCallInteration:(id)cCenter {
	if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_9_x_Max) {
		CTCallCenter *ct = (CTCallCenter *)cCenter;
		// pause current call, if any
		LinphoneCall *call = linphone_core_get_current_call(theLinphoneCore);
		if ([ct currentCalls] != nil) {
			if (call) {
				LOGI(@"Pausing SIP call because GSM call");
				_speakerBeforePause = _speakerEnabled;
				linphone_call_pause(call);
				[self startCallPausedLongRunningTask];
			} else if (linphone_core_is_in_conference(theLinphoneCore)) {
				LOGI(@"Leaving conference call because GSM call");
				linphone_core_leave_conference(theLinphoneCore);
				[self startCallPausedLongRunningTask];
			}
		} // else nop, keep call in paused state
	}
}

- (NSString *)contactFilter {
	NSString *filter = @"*";
	if ([self lpConfigBoolForKey:@"contact_filter_on_default_domain"]) {
		LinphoneProxyConfig *proxy_cfg = linphone_core_get_default_proxy_config(theLinphoneCore);
		if (proxy_cfg && linphone_proxy_config_get_addr(proxy_cfg)) {
			return [NSString stringWithCString:linphone_proxy_config_get_domain(proxy_cfg)
									  encoding:[NSString defaultCStringEncoding]];
		}
	}
	return filter;
}

#pragma mark - InApp Purchase events

- (void)inappReady:(NSNotification *)notif {
	// Query our in-app server to retrieve InApp purchases
	//[_iapManager retrievePurchases];
}

#pragma mark -

- (void)removeAllAccounts {
	linphone_core_clear_proxy_config(LC);
	linphone_core_clear_all_auth_info(LC);
}

+ (BOOL)isMyself:(const LinphoneAddress *)addr {
	if (!addr)
		return NO;

	const MSList *it = linphone_core_get_proxy_config_list(LC);
	while (it) {
		if (linphone_address_weak_equal(addr, linphone_proxy_config_get_identity_address(it->data))) {
			return YES;
		}
		it = it->next;
	}
	return NO;
}

// ugly hack to export symbol from liblinphone so that they are available for the linphoneTests target
// linphoneTests target do not link with liblinphone but instead dynamically link with ourself which is
// statically linked with liblinphone, so we must have exported required symbols from the library to
// have them available in linphoneTests
// DO NOT INVOKE THIS METHOD
- (void)exportSymbolsForUITests {
	linphone_address_set_header(NULL, NULL, NULL);
}

- (void)checkNewVersion {
    if (!CHECK_VERSION_UPDATE)
        return;
    if (theLinphoneCore == nil)
        return;
    NSString *curVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    const char *curVersionCString = [curVersion cStringUsingEncoding:NSUTF8StringEncoding];
    linphone_core_check_for_update(theLinphoneCore, curVersionCString);
}

- (void)loadAvatar {
    NSString *assetId = [self lpConfigStringForKey:@"avatar"];
    __block UIImage *ret = nil;
    if (assetId) {
        PHFetchResult<PHAsset *> *assets = [PHAsset fetchAssetsWithLocalIdentifiers:[NSArray arrayWithObject:assetId] options:nil];
        if (![assets firstObject]) {
            LOGE(@"Can't fetch avatar image.");
        }
        PHAsset *asset = [assets firstObject];
        // load avatar synchronously so that we can return UIIMage* directly - since we are
        // only using thumbnail, it must be pretty fast to fetch even without cache.
        PHImageRequestOptions *options = [[PHImageRequestOptions alloc] init];
        options.synchronous = TRUE;
        [[PHImageManager defaultManager] requestImageForAsset:asset targetSize:PHImageManagerMaximumSize contentMode:PHImageContentModeDefault options:options
                                                resultHandler:^(UIImage *image, NSDictionary * info) {
                                                    if (image)
                                                        ret = [UIImage UIImageThumbnail:image thumbSize:150];
                                                    else
                                                        LOGE(@"Can't read avatar");
                                                }];
    }
    
    if (!ret) {
        ret = [UIImage imageNamed:@"avatar.png"];
    }
    _avatar = ret;
}

@end
