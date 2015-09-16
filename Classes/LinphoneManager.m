
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
#import <AudioToolbox/AudioToolbox.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <CoreTelephony/CTCallCenter.h>
#import <SystemConfiguration/CaptiveNetwork.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>

#import "LinphoneManager.h"
#import "LinphoneCoreSettingsStore.h"
#import "Utils/FileTransferDelegate.h"

#include "linphone/linphonecore_utils.h"
#include "linphone/lpconfig.h"
#include "mediastreamer2/mscommon.h"

#import "LinphoneIOSVersion.h"

#import <AVFoundation/AVAudioPlayer.h>

#define LINPHONE_LOGS_MAX_ENTRY 5000

static void audioRouteChangeListenerCallback(void *inUserData,					  // 1
											 AudioSessionPropertyID inPropertyID, // 2
											 UInt32 inPropertyValueSize,		  // 3
											 const void *inPropertyValue		  // 4
											 );
static LinphoneCore *theLinphoneCore = nil;
static LinphoneManager *theLinphoneManager = nil;

NSString *const LINPHONERC_APPLICATION_KEY = @"app";

NSString *const kLinphoneCoreUpdate = @"LinphoneCoreUpdate";
NSString *const kLinphoneDisplayStatusUpdate = @"LinphoneDisplayStatusUpdate";
NSString *const kLinphoneTextReceived = @"LinphoneTextReceived";
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
NSString *const kLinphoneFileTransferSendUpdate = @"LinphoneFileTransferSendUpdate";
NSString *const kLinphoneFileTransferRecvUpdate = @"LinphoneFileTransferRecvUpdate";

const int kLinphoneAudioVbrCodecDefaultBitrate = 36; /*you can override this from linphonerc or linphonerc-factory*/

extern void libmsilbc_init(void);
extern void libmsamr_init(void);
extern void libmsx264_init(void);
extern void libmsopenh264_init(void);
extern void libmssilk_init(void);
extern void libmsbcg729_init(void);
extern void libmswebrtc_init(void);

#define FRONT_CAM_NAME                                                                                                 \
	"AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:1" /*"AV Capture: Front Camera"*/
#define BACK_CAM_NAME                                                                                                  \
	"AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:0" /*"AV Capture: Back Camera"*/

NSString *const kLinphoneOldChatDBFilename = @"chat_database.sqlite";
NSString *const kLinphoneInternalChatDBFilename = @"linphone_chats.db";

@implementation LinphoneCallAppData
- (id)init {
	if ((self = [super init])) {
		self->batteryWarningShown = FALSE;
		self->notification = nil;
		self->videoRequested = FALSE;
		self->userInfos = [[NSMutableDictionary alloc] init];
	}
	return self;
}

@end

@interface LinphoneManager ()
@property(strong, nonatomic) AVAudioPlayer *messagePlayer;
@end

@implementation LinphoneManager

@synthesize connectivity;
@synthesize network;
@synthesize frontCamId;
@synthesize backCamId;
@synthesize database;
@synthesize fastAddressBook;
@synthesize pushNotificationToken;
@synthesize sounds;
@synthesize logs;
@synthesize speakerEnabled;
@synthesize bluetoothAvailable;
@synthesize bluetoothEnabled;
@synthesize photoLibrary;
@synthesize tunnelMode;
@synthesize silentPushCompletion;
@synthesize wasRemoteProvisioned;
@synthesize configDb;

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
												   {"vp8", 90000, "vp8_preference"},
												   {"mpeg4-generic", 16000, "aaceld_16k_preference"},
												   {"mpeg4-generic", 22050, "aaceld_22k_preference"},
												   {"mpeg4-generic", 32000, "aaceld_32k_preference"},
												   {"mpeg4-generic", 44100, "aaceld_44k_preference"},
												   {"mpeg4-generic", 48000, "aaceld_48k_preference"},
												   {"opus", 48000, "opus_preference"},
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
			&&
			strcmp(codec_pref_table[i].prefname, "h264_preference") != 0 &&
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
		AudioSessionInitialize(NULL, NULL, NULL, NULL);
		OSStatus lStatus = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange,
														   audioRouteChangeListenerCallback, (__bridge void *)(self));
		if (lStatus) {
			LOGE(@"cannot register route change handler [%ld]", lStatus);
		}

		NSString *path = [[NSBundle mainBundle] pathForResource:@"msg" ofType:@"wav"];
		self.messagePlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL URLWithString:path] error:nil];

		sounds.vibrate = kSystemSoundID_Vibrate;

		logs = [[NSMutableArray alloc] init];
		database = NULL;
		speakerEnabled = FALSE;
		bluetoothEnabled = FALSE;
		tunnelMode = FALSE;

		_fileTransferDelegates = [[NSMutableArray alloc] init];

		pushCallIDs = [[NSMutableArray alloc] init];
		photoLibrary = [[ALAssetsLibrary alloc] init];
		self->_isTesting = [LinphoneManager isRunningTests];

		[self renameDefaultSettings];
		[self copyDefaultSettings];
		[self overrideDefaultSettings];

		// set default values for first boot
		if ([self lpConfigStringForKey:@"debugenable_preference"] == nil) {
#ifdef DEBUG
			[self lpConfigSetBool:TRUE forKey:@"debugenable_preference"];
#else
			[self lpConfigSetBool:FALSE forKey:@"debugenable_preference"];
#endif
		}

		[self migrateFromUserPrefs];
	}
	return self;
}

- (void)dealloc {

	OSStatus lStatus = AudioSessionRemovePropertyListenerWithUserData(
		kAudioSessionProperty_AudioRouteChange, audioRouteChangeListenerCallback, (__bridge void *)(self));
	if (lStatus) {
		LOGE(@"cannot un register route change handler [%ld]", lStatus);
	}

	[[NSNotificationCenter defaultCenter] removeObserver:self forKeyPath:kLinphoneGlobalStateUpdate];
	[[NSNotificationCenter defaultCenter] removeObserver:self forKeyPath:kLinphoneConfiguringStateUpdate];
}

- (void)silentPushFailed:(NSTimer *)timer {
	if (silentPushCompletion) {
		LOGI(@"silentPush failed, silentPushCompletion block: %p", silentPushCompletion);
		silentPushCompletion(UIBackgroundFetchResultNoData);
		silentPushCompletion = nil;
	}
}

#pragma mark - Migration

static int check_should_migrate_images(void *data, int argc, char **argv, char **cnames) {
	*((BOOL *)data) = TRUE;
	return 0;
}

- (BOOL)migrateChatDBIfNeeded:(LinphoneCore *)lc {
	sqlite3 *newDb;
	char *errMsg;
	NSError *error;
	NSString *oldDbPath = [LinphoneManager documentFile:kLinphoneOldChatDBFilename];
	NSString *newDbPath = [LinphoneManager documentFile:kLinphoneInternalChatDBFilename];
	BOOL shouldMigrate = [[NSFileManager defaultManager] fileExistsAtPath:oldDbPath];
	BOOL shouldMigrateImages = FALSE;
	LinphoneProxyConfig *default_proxy;
	const char *identity = NULL;
	BOOL migrated = FALSE;
	char *attach_stmt = NULL;

	linphone_core_get_default_proxy(lc, &default_proxy);

	if (sqlite3_open([newDbPath UTF8String], &newDb) != SQLITE_OK) {
		LOGE(@"Can't open \"%@\" sqlite3 database.", newDbPath);
		return FALSE;
	}

	const char *check_appdata =
		"SELECT url,message FROM history WHERE url LIKE 'assets-library%' OR message LIKE 'assets-library%' LIMIT 1;";
	// will set "needToMigrateImages to TRUE if a result comes by
	sqlite3_exec(newDb, check_appdata, check_should_migrate_images, &shouldMigrateImages, NULL);
	if (!shouldMigrate && !shouldMigrateImages) {
		sqlite3_close(newDb);
		return FALSE;
	}

	LOGI(@"Starting migration procedure");

	if (shouldMigrate) {

		// attach old database to the new one:
		attach_stmt = sqlite3_mprintf("ATTACH DATABASE %Q AS oldchats", [oldDbPath UTF8String]);
		if (sqlite3_exec(newDb, attach_stmt, NULL, NULL, &errMsg) != SQLITE_OK) {
			LOGE(@"Can't attach old chat table, error[%s] ", errMsg);
			sqlite3_free(errMsg);
			goto exit_dbmigration;
		}

		// migrate old chats to the new db. The iOS stores timestamp in UTC already, so we can directly put it in the
		// 'utc' field and set 'time' to -1
		const char *migration_statement =
			"INSERT INTO history (localContact,remoteContact,direction,message,utc,read,status,time) "
			"SELECT localContact,remoteContact,direction,message,time,read,state,'-1' FROM oldchats.chat";

		if (sqlite3_exec(newDb, migration_statement, NULL, NULL, &errMsg) != SQLITE_OK) {
			LOGE(@"DB migration failed, error[%s] ", errMsg);
			sqlite3_free(errMsg);
			goto exit_dbmigration;
		}

		// invert direction of old messages, because iOS was storing the direction flag incorrectly
		const char *invert_direction = "UPDATE history SET direction = NOT direction";
		if (sqlite3_exec(newDb, invert_direction, NULL, NULL, &errMsg) != SQLITE_OK) {
			LOGE(@"Inverting direction failed, error[%s]", errMsg);
			sqlite3_free(errMsg);
			goto exit_dbmigration;
		}

		// replace empty from: or to: by the current identity.
		if (default_proxy) {
			identity = linphone_proxy_config_get_identity(default_proxy);
		}
		if (!identity) {
			identity = "sip:unknown@sip.linphone.org";
		}

		char *from_conversion =
			sqlite3_mprintf("UPDATE history SET localContact = %Q WHERE localContact = ''", identity);
		if (sqlite3_exec(newDb, from_conversion, NULL, NULL, &errMsg) != SQLITE_OK) {
			LOGE(@"FROM conversion failed, error[%s] ", errMsg);
			sqlite3_free(errMsg);
		}
		sqlite3_free(from_conversion);

		char *to_conversion =
			sqlite3_mprintf("UPDATE history SET remoteContact = %Q WHERE remoteContact = ''", identity);
		if (sqlite3_exec(newDb, to_conversion, NULL, NULL, &errMsg) != SQLITE_OK) {
			LOGE(@"DB migration failed, error[%s] ", errMsg);
			sqlite3_free(errMsg);
		}
		sqlite3_free(to_conversion);
	}

	// local image paths were stored in the 'message' field historically. They were
	// very temporarily stored in the 'url' field, and now we migrated them to a JSON-
	// encoded field. These are the migration steps to migrate them.

	// move already stored images from the messages to the appdata JSON field
	const char *assetslib_migration = "UPDATE history SET appdata='{\"localimage\":\"'||message||'\"}' , message='' "
									  "WHERE message LIKE 'assets-library%'";
	if (sqlite3_exec(newDb, assetslib_migration, NULL, NULL, &errMsg) != SQLITE_OK) {
		LOGE(@"Assets-history migration for MESSAGE failed, error[%s] ", errMsg);
		sqlite3_free(errMsg);
	}

	// move already stored images from the url to the appdata JSON field
	const char *assetslib_migration_fromurl =
		"UPDATE history SET appdata='{\"localimage\":\"'||url||'\"}' , url='' WHERE url LIKE 'assets-library%'";
	if (sqlite3_exec(newDb, assetslib_migration_fromurl, NULL, NULL, &errMsg) != SQLITE_OK) {
		LOGE(@"Assets-history migration for URL failed, error[%s] ", errMsg);
		sqlite3_free(errMsg);
	}

	// We will lose received messages with remote url, they will be displayed in plain. We can't do much for them..
	migrated = TRUE;

exit_dbmigration:

	if (attach_stmt)
		sqlite3_free(attach_stmt);

	sqlite3_close(newDb);

	// in any case, we should remove the old chat db
	if (shouldMigrate && ![[NSFileManager defaultManager] removeItemAtPath:oldDbPath error:&error]) {
		LOGE(@"Could not remove old chat DB: %@", error);
	}

	LOGI(@"Message storage migration finished: success = %@", migrated ? @"TRUE" : @"FALSE");
	return migrated;
}

- (void)migrateFromUserPrefs {
	static NSString *migration_flag = @"userpref_migration_done";

	if (configDb == nil)
		return;

	if ([self lpConfigIntForKey:migration_flag withDefault:0]) {
		LOGI(@"UserPrefs migration already performed, skip");
		return;
	}

	NSDictionary *defaults = [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
	NSArray *defaults_keys = [defaults allKeys];
	NSDictionary *values =
		@{ @"backgroundmode_preference" : @YES,
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
	// we need to proceed to the migration *after* the chat database was opened, so that we know it is in consistent
	// state
	NSString *chatDBFileName = [LinphoneManager documentFile:kLinphoneInternalChatDBFilename];
	if ([self migrateChatDBIfNeeded:theLinphoneCore]) {
		// if a migration was performed, we should reinitialize the chat database
		linphone_core_set_chat_database_path(theLinphoneCore,
											 [chatDBFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
	}

	/* AVPF migration */
	if ([self lpConfigBoolForKey:@"avpf_migration_done"] == FALSE) {
		const MSList *proxies = linphone_core_get_proxy_config_list(theLinphoneCore);
		while (proxies) {
			LinphoneProxyConfig *proxy = (LinphoneProxyConfig *)proxies->data;
			const char *addr = linphone_proxy_config_get_addr(proxy);
			// we want to enable AVPF for the proxies
			if (addr && strstr(addr, "sip.linphone.org") != 0) {
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
			if (addr && strstr(addr, "sip.linphone.org") != 0) {
				LOGI(@"Migrating proxy config to send quality report");
				linphone_proxy_config_set_quality_reporting_collector(proxy, "sip:voip-metrics@sip.linphone.org");
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
		LOGI(@"Migrating sharing server url from %s to %s",
			 linphone_core_get_file_transfer_server([LinphoneManager getLc]), newURL);
		linphone_core_set_file_transfer_server([LinphoneManager getLc], newURL);
		[self lpConfigSetBool:TRUE forKey:@"file_transfer_migration_done"];
	}
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

struct _entry_data {
	const LpConfig *conf;
	const char *section;
};

static void dump_entry(const char *entry, void *data) {
	struct _entry_data *d = (struct _entry_data *)data;
	const char *value = lp_config_get_string(d->conf, d->section, entry, "");
	LOGI(@"%s=%s", entry, value);
}

static void dump_section(const char *section, void *data) {
	LOGI(@"[%s]", section);
	struct _entry_data d = {(const LpConfig *)data, section};
	lp_config_for_each_entry((const LpConfig *)data, section, dump_entry, &d);
}

+ (void)dumpLCConfig {
	if (theLinphoneCore) {
		LpConfig *conf = [LinphoneManager instance].configDb;
		lp_config_for_each_section(conf, dump_section, conf);
	}
}

#pragma mark - Logs Functions handlers
static void linphone_iphone_log_user_info(struct _LinphoneCore *lc, const char *message) {
	linphone_iphone_log_handler(ORTP_MESSAGE, message, NULL);
}
static void linphone_iphone_log_user_warning(struct _LinphoneCore *lc, const char *message) {
	linphone_iphone_log_handler(ORTP_WARNING, message, NULL);
}

#pragma mark - Display Status Functions

- (void)displayStatus:(NSString *)message {
	// Post event
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneDisplayStatusUpdate
														object:self
													  userInfo:@{
														  @"message" : message
													  }];
}

static void linphone_iphone_display_status(struct _LinphoneCore *lc, const char *message) {
	NSString *status = [[NSString alloc] initWithCString:message encoding:[NSString defaultCStringEncoding]];
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) displayStatus:status];
}

#pragma mark - Call State Functions

- (void)localNotifContinue:(NSTimer *)timer {
	UILocalNotification *notif = [timer userInfo];
	if (notif) {
		LOGI(@"cancelling/presenting local notif");
		[[UIApplication sharedApplication] cancelLocalNotification:notif];
		[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
	}
}

- (void)onCall:(LinphoneCall *)call StateChanged:(LinphoneCallState)state withMessage:(const char *)message {

	// Handling wrapper
	LinphoneCallAppData *data = (__bridge LinphoneCallAppData *)linphone_call_get_user_data(call);
	if (!data) {
		data = [[LinphoneCallAppData alloc] init];
		linphone_call_set_user_data(call, (void *)CFBridgingRetain(data));
	}

	if (silentPushCompletion) {

		// we were woken up by a silent push. Call the completion handler with NEWDATA
		// so that the push is notified to the user
		LOGI(@"onCall - handler %p", silentPushCompletion);
		silentPushCompletion(UIBackgroundFetchResultNewData);
		silentPushCompletion = nil;
	}

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	NSString *address = nil;
	if (addr != NULL) {
		BOOL useLinphoneAddress = true;
		// contact name
		char *lAddress = linphone_address_as_string_uri_only(addr);
		if (lAddress) {
			NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:lAddress]];
			ABRecordRef contact = [fastAddressBook getContact:normalizedSipAddress];
			if (contact) {
				address = [FastAddressBook getContactDisplayName:contact];
				useLinphoneAddress = false;
			}
			ms_free(lAddress);
		}
		if (useLinphoneAddress) {
			const char *lDisplayName = linphone_address_get_display_name(addr);
			const char *lUserName = linphone_address_get_username(addr);
			if (lDisplayName)
				address = [NSString stringWithUTF8String:lDisplayName];
			else if (lUserName)
				address = [NSString stringWithUTF8String:lUserName];
		}
	}
	if (address == nil) {
		address = NSLocalizedString(@"Unknown", nil);
	}

	if (state == LinphoneCallIncomingReceived) {

		/*first step is to re-enable ctcall center*/
		CTCallCenter *lCTCallCenter = [[CTCallCenter alloc] init];

		/*should we reject this call ?*/
		if ([lCTCallCenter currentCalls] != nil) {
			char *tmp = linphone_call_get_remote_address_as_string(call);
			if (tmp) {
				LOGI(@"Mobile call ongoing... rejecting call from [%s]", tmp);
				ms_free(tmp);
			}
			linphone_core_decline_call(theLinphoneCore, call, LinphoneReasonBusy);
			return;
		}

		if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {

			LinphoneCallLog *callLog = linphone_call_get_call_log(call);
			NSString *callId = [NSString stringWithUTF8String:linphone_call_log_get_call_id(callLog)];

			if (![[LinphoneManager instance] popPushCallID:callId]) {
				// case where a remote notification is not already received
				// Create a new local notification
				data->notification = [[UILocalNotification alloc] init];
				if (data->notification) {

					// iOS8 doesn't need the timer trick for the local notification.
					if ([[UIDevice currentDevice].systemVersion floatValue] >= 8 &&
						[self lpConfigBoolForKey:@"repeat_call_notification"] == NO) {
						data->notification.soundName = @"ring.caf";
						data->notification.category = @"incoming_call";
					} else {
						data->notification.soundName = @"shortring.caf";
						data->timer = [NSTimer scheduledTimerWithTimeInterval:4.0
																	   target:self
																	 selector:@selector(localNotifContinue:)
																	 userInfo:data->notification
																	  repeats:TRUE];
					}

					data->notification.repeatInterval = 0;

					data->notification.alertBody =
						[NSString stringWithFormat:NSLocalizedString(@"IC_MSG", nil), address];
					data->notification.alertAction = NSLocalizedString(@"Answer", nil);
					data->notification.userInfo = @{ @"callId" : callId, @"timer" : [NSNumber numberWithInt:1] };
					data->notification.applicationIconBadgeNumber = 1;

					[[UIApplication sharedApplication] presentLocalNotificationNow:data->notification];

					if (!incallBgTask) {
						incallBgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
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

	// we keep the speaker auto-enabled state in this static so that we don't
	// force-enable it on ICE re-invite if the user disabled it.
	static BOOL speaker_already_enabled = FALSE;

	// Disable speaker when no more call
	if ((state == LinphoneCallEnd || state == LinphoneCallError)) {
		speaker_already_enabled = FALSE;
		if (linphone_core_get_calls_nb(theLinphoneCore) == 0) {
			[self setSpeakerEnabled:FALSE];
			[self removeCTCallCenterCb];
			bluetoothAvailable = FALSE;
			bluetoothEnabled = FALSE;
			/*IOS specific*/
			linphone_core_start_dtmf_stream(theLinphoneCore);
		}
		if (incallBgTask) {
			[[UIApplication sharedApplication] endBackgroundTask:incallBgTask];
			incallBgTask = 0;
		}
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
				notification.alertBody =
					[NSString stringWithFormat:NSLocalizedString(@"You missed a call from %@", nil), address];
				notification.alertAction = NSLocalizedString(@"Show", nil);
				notification.userInfo = [NSDictionary
					dictionaryWithObject:[NSString stringWithUTF8String:linphone_call_log_get_call_id(log)]
								  forKey:@"callLog"];
				[[UIApplication sharedApplication] presentLocalNotificationNow:notification];
			}
		}
	}

	if (state == LinphoneCallReleased) {
		if (data != NULL) {
			linphone_call_set_user_data(call, NULL);
			CFBridgingRelease((__bridge CFTypeRef)(data));
		}
	}

	// Enable speaker when video
	if (state == LinphoneCallIncomingReceived || state == LinphoneCallOutgoingInit || state == LinphoneCallConnected ||
		state == LinphoneCallStreamsRunning) {
		if (linphone_call_params_video_enabled(linphone_call_get_current_params(call)) && !speaker_already_enabled) {
			[self setSpeakerEnabled:TRUE];
			speaker_already_enabled = TRUE;
		}
	}

	if (state == LinphoneCallConnected && !mCallCenter) {
		/*only register CT call center CB for connected call*/
		[self setupGSMInteraction];
	}
	// Post event
	NSDictionary *dict = @{
		@"call" : [NSValue valueWithPointer:call],
		@"state" : [NSNumber numberWithInt:state],
		@"message" : [NSString stringWithUTF8String:message]
	};
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneCallUpdate object:self userInfo:dict];
}

static void linphone_iphone_call_state(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state,
									   const char *message) {
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onCall:call StateChanged:state withMessage:message];
}

#pragma mark - Transfert State Functions

static void linphone_iphone_transfer_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state) {
}

#pragma mark - Global state change

static void linphone_iphone_global_state_changed(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message) {
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onGlobalStateChanged:gstate withMessage:message];
}

- (void)onGlobalStateChanged:(LinphoneGlobalState)state withMessage:(const char *)message {
	LOGI(@"onGlobalStateChanged: %d (message: %s)", state, message);

	NSDictionary *dict = [NSDictionary
		dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:state], @"state",
									 [NSString stringWithUTF8String:message ? message : ""], @"message", nil];

	// dispatch the notification asynchronously
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneGlobalStateUpdate object:self userInfo:dict];
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
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onConfiguringStatusChanged:status withMessage:message];
}

- (void)onConfiguringStatusChanged:(LinphoneConfiguringState)status withMessage:(const char *)message {
	LOGI(@"onConfiguringStatusChanged: %s %@", linphone_configuring_state_to_string(status),
		 message ? [NSString stringWithFormat:@"(message: %s)", message] : @"");

	NSDictionary *dict = [NSDictionary
		dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:status], @"state",
									 [NSString stringWithUTF8String:message ? message : ""], @"message", nil];

	// dispatch the notification asynchronously
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneConfiguringStateUpdate
														  object:self
														userInfo:dict];
	});
}

- (void)configuringStateChangedNotificationHandler:(NSNotification *)notif {
	if ((LinphoneConfiguringState)[[[notif userInfo] valueForKey:@"state"] integerValue] ==
		LinphoneConfiguringSuccessful) {
		wasRemoteProvisioned = TRUE;
	} else {
		wasRemoteProvisioned = FALSE;
	}
}

#pragma mark - Registration State Functions

- (void)onRegister:(LinphoneCore *)lc
			   cfg:(LinphoneProxyConfig *)cfg
			 state:(LinphoneRegistrationState)state
		   message:(const char *)message {
	LOGI(@"New registration state: %s (message: %s)", linphone_registration_state_to_string(state), message);

	// Post event
	NSDictionary *dict = [NSDictionary
		dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:state], @"state", [NSValue valueWithPointer:cfg], @"cfg",
									 [NSString stringWithUTF8String:message], @"message", nil];
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneRegistrationUpdate object:self userInfo:dict];
}

static void linphone_iphone_registration_state(LinphoneCore *lc, LinphoneProxyConfig *cfg,
											   LinphoneRegistrationState state, const char *message) {
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onRegister:lc cfg:cfg state:state message:message];
}

#pragma mark - Text Received Functions

- (void)onMessageReceived:(LinphoneCore *)lc room:(LinphoneChatRoom *)room message:(LinphoneChatMessage *)msg {

	if (silentPushCompletion) {

		// we were woken up by a silent push. Call the completion handler with NEWDATA
		// so that the push is notified to the user
		LOGI(@"onMessageReceived - handler %p", silentPushCompletion);
		silentPushCompletion(UIBackgroundFetchResultNewData);
		silentPushCompletion = nil;
	}
	const LinphoneAddress *remoteAddress = linphone_chat_message_get_from_address(msg);
	char *c_address = linphone_address_as_string_uri_only(remoteAddress);
	NSString *address = [NSString stringWithUTF8String:c_address];
	NSString *remote_uri = [NSString stringWithUTF8String:c_address];
	const char *call_id = linphone_chat_message_get_custom_header(msg, "Call-ID");
	NSString *callID = [NSString stringWithUTF8String:call_id];

	ms_free(c_address);

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {

		ABRecordRef contact = [fastAddressBook getContact:address];
		if (contact) {
			address = [FastAddressBook getContactDisplayName:contact];
		} else {
			if ([[LinphoneManager instance] lpConfigBoolForKey:@"show_contacts_emails_preference"] == true) {
				LinphoneAddress *linphoneAddress =
					linphone_address_new([address cStringUsingEncoding:[NSString defaultCStringEncoding]]);
				address = [NSString stringWithUTF8String:linphone_address_get_username(linphoneAddress)];
				linphone_address_destroy(linphoneAddress);
			}
		}
		if (address == nil) {
			address = NSLocalizedString(@"Unknown", nil);
		}

		// Create a new notification
		UILocalNotification *notif = [[UILocalNotification alloc] init];
		if (notif) {
			notif.repeatInterval = 0;
			if ([[UIDevice currentDevice].systemVersion floatValue] >= 8) {
				notif.category = @"incoming_msg";
			}
			notif.alertBody = [NSString stringWithFormat:NSLocalizedString(@"IM_MSG", nil), address];
			notif.alertAction = NSLocalizedString(@"Show", nil);
			notif.soundName = @"msg.caf";
			notif.userInfo = @{ @"from" : address, @"from_addr" : remote_uri, @"call-id" : callID };

			[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
		}
	}

	// Post event
	NSDictionary *dict = @{
		@"room" : [NSValue valueWithPointer:room],
		@"from_address" : [NSValue valueWithPointer:linphone_chat_message_get_from_address(msg)],
		@"message" : [NSValue valueWithPointer:msg],
		@"call-id" : callID
	};

	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self userInfo:dict];
}

static void linphone_iphone_message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message) {
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onMessageReceived:lc room:room message:message];
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
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneNotifyReceived object:self userInfo:dict];
}

static void linphone_iphone_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event,
											const LinphoneContent *body) {
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onNotifyReceived:lc
																			event:lev
																	  notifyEvent:notified_event
																		  content:body];
}

#pragma mark - Message composition start

- (void)onMessageComposeReceived:(LinphoneCore *)core forRoom:(LinphoneChatRoom *)room {
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextComposeEvent
														object:self
													  userInfo:@{
														  @"room" : [NSValue valueWithPointer:room]
													  }];
}

static void linphone_iphone_is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
	[(__bridge LinphoneManager *)linphone_core_get_user_data(lc) onMessageComposeReceived:lc forRoom:room];
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
	LOGI(@"Network connection flags:");
	if (flags == 0)
		LOGI(@"no flags.");
	if (flags & kSCNetworkReachabilityFlagsTransientConnection)
		LOGI(@"kSCNetworkReachabilityFlagsTransientConnection");
	if (flags & kSCNetworkReachabilityFlagsReachable)
		LOGI(@"kSCNetworkReachabilityFlagsReachable");
	if (flags & kSCNetworkReachabilityFlagsConnectionRequired)
		LOGI(@"kSCNetworkReachabilityFlagsConnectionRequired");
	if (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)
		LOGI(@"kSCNetworkReachabilityFlagsConnectionOnTraffic");
	if (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)
		LOGI(@"kSCNetworkReachabilityFlagsConnectionOnDemand");
	if (flags & kSCNetworkReachabilityFlagsIsLocalAddress)
		LOGI(@"kSCNetworkReachabilityFlagsIsLocalAddress");
	if (flags & kSCNetworkReachabilityFlagsIsDirect)
		LOGI(@"kSCNetworkReachabilityFlagsIsDirect");
	if (flags & kSCNetworkReachabilityFlagsIsWWAN)
		LOGI(@"kSCNetworkReachabilityFlagsIsWWAN");
}

static void networkReachabilityNotification(CFNotificationCenterRef center, void *observer, CFStringRef name,
											const void *object, CFDictionaryRef userInfo) {
	LinphoneManager *mgr = [LinphoneManager instance];
	SCNetworkReachabilityFlags flags;

	// for an unknown reason, we are receiving multiple time the notification, so
	// we will skip each time the SSID did not change
	NSString *newSSID = [LinphoneManager getCurrentWifiSSID];
	if ([newSSID compare:mgr.SSID] == NSOrderedSame)
		return;

	mgr.SSID = newSSID;

	if (SCNetworkReachabilityGetFlags([mgr getProxyReachability], &flags)) {
		networkReachabilityCallBack([mgr getProxyReachability], flags, nil);
	}
}

void networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void *nilCtx) {
	showNetworkFlags(flags);
	LinphoneManager *lm = [LinphoneManager instance];
	SCNetworkReachabilityFlags networkDownFlags = kSCNetworkReachabilityFlagsConnectionRequired |
												  kSCNetworkReachabilityFlagsConnectionOnTraffic |
												  kSCNetworkReachabilityFlagsConnectionOnDemand;

	if (theLinphoneCore != nil) {
		LinphoneProxyConfig *proxy;
		linphone_core_get_default_proxy(theLinphoneCore, &proxy);

		struct NetworkReachabilityContext *ctx = nilCtx ? ((struct NetworkReachabilityContext *)nilCtx) : 0;
		if ((flags == 0) || (flags & networkDownFlags)) {
			linphone_core_set_network_reachable(theLinphoneCore, false);
			lm.connectivity = none;
			[LinphoneManager kickOffNetworkConnection];
		} else {
			LinphoneTunnel *tunnel = linphone_core_get_tunnel([LinphoneManager getLc]);
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
				if (tunnel)
					linphone_tunnel_reconnect(tunnel);
				// connectivity has changed
				linphone_core_set_network_reachable(theLinphoneCore, false);
				if (newConnectivity == wwan && proxy && isWifiOnly) {
					linphone_proxy_config_expires(proxy, 0);
				}
				linphone_core_set_network_reachable(theLinphoneCore, true);
				linphone_core_iterate(theLinphoneCore);
				LOGI(@"Network connectivity changed to type [%s]", (newConnectivity == wifi ? "wifi" : "wwan"));
			}
			lm.connectivity = newConnectivity;
			switch (lm.tunnelMode) {
			case tunnel_wwan:
				linphone_tunnel_enable(tunnel, lm.connectivity == wwan);
				break;
			case tunnel_auto:
				linphone_tunnel_auto_detect(tunnel);
				break;
			default:
				// nothing to do
				break;
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
		CTTelephonyNetworkInfo *info = [[CTTelephonyNetworkInfo alloc] init];
		NSString *currentRadio = info.currentRadioAccessTechnology;
		if ([currentRadio isEqualToString:CTRadioAccessTechnologyEdge]) {
			return network_2g;
		} else if ([currentRadio isEqualToString:CTRadioAccessTechnologyLTE]) {
			return network_4g;
		}
		return network_3g;
	}
}

#pragma mark - VTable

static LinphoneCoreVTable linphonec_vtable = {.show = NULL,
											  .call_state_changed =
												  (LinphoneCoreCallStateChangedCb)linphone_iphone_call_state,
											  .registration_state_changed = linphone_iphone_registration_state,
											  .notify_presence_received = NULL,
											  .new_subscription_requested = NULL,
											  .auth_info_requested = NULL,
											  .display_status = linphone_iphone_display_status,
											  .display_message = linphone_iphone_log_user_info,
											  .display_warning = linphone_iphone_log_user_warning,
											  .display_url = NULL,
											  .text_received = NULL,
											  .message_received = linphone_iphone_message_received,
											  .dtmf_received = NULL,
											  .transfer_state_changed = linphone_iphone_transfer_state_changed,
											  .is_composing_received = linphone_iphone_is_composing_received,
											  .configuring_status = linphone_iphone_configuring_status_changed,
											  .global_state_changed = linphone_iphone_global_state_changed,
											  .notify_received = linphone_iphone_notify_received};

#pragma mark -

// scheduling loop
- (void)iterate {
	linphone_core_iterate(theLinphoneCore);
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

	// get default config from bundle
	NSString *zrtpSecretsFileName = [LinphoneManager documentFile:@"zrtp_secrets"];
	NSString *chatDBFileName = [LinphoneManager documentFile:kLinphoneInternalChatDBFilename];
	const char *lRootCa =
		[[LinphoneManager bundleFile:@"rootca.pem"] cStringUsingEncoding:[NSString defaultCStringEncoding]];

	linphone_core_set_user_agent(theLinphoneCore,
								 [[[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"]
									 stringByAppendingString:@"Iphone"] UTF8String],
								 LINPHONE_IOS_VERSION);

	_contactSipField = [self lpConfigStringForKey:@"contact_im_type_value" withDefault:@"SIP"];

	if (fastAddressBook == nil) {
		fastAddressBook = [[FastAddressBook alloc] init];
	}

	linphone_core_set_root_ca(theLinphoneCore, lRootCa);
	// Set audio assets
	const char *lRing =
		[[LinphoneManager bundleFile:@"ring.wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(theLinphoneCore, lRing);
	const char *lRingBack =
		[[LinphoneManager bundleFile:@"ringback.wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(theLinphoneCore, lRingBack);
	const char *lPlay =
		[[LinphoneManager bundleFile:@"hold.wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_play_file(theLinphoneCore, lPlay);

	linphone_core_set_zrtp_secrets_file(theLinphoneCore,
										[zrtpSecretsFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
	linphone_core_set_chat_database_path(theLinphoneCore,
										 [chatDBFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]);

	[self migrationLinphoneSettings];

	[self setupNetworkReachabilityCallback];

	NSString *path = [LinphoneManager bundleFile:@"nowebcamCIF.jpg"];
	if (path) {
		const char *imagePath = [path cStringUsingEncoding:[NSString defaultCStringEncoding]];
		LOGI(@"Using '%s' as source image for no webcam", imagePath);
		linphone_core_set_static_picture(theLinphoneCore, imagePath);
	}

	/*DETECT cameras*/
	frontCamId = backCamId = nil;
	char **camlist = (char **)linphone_core_get_video_devices(theLinphoneCore);
	for (char *cam = *camlist; *camlist != NULL; cam = *++camlist) {
		if (strcmp(FRONT_CAM_NAME, cam) == 0) {
			frontCamId = cam;
			// great set default cam to front
			linphone_core_set_video_device(theLinphoneCore, cam);
		}
		if (strcmp(BACK_CAM_NAME, cam) == 0) {
			backCamId = cam;
		}
	}

	if (![LinphoneManager isNotIphone3G]) {
		PayloadType *pt = linphone_core_find_payload_type(theLinphoneCore, "SILK", 24000, -1);
		if (pt) {
			linphone_core_enable_payload_type(theLinphoneCore, pt, FALSE);
			LOGW(@"SILK/24000 and video disabled on old iPhone 3G");
		}
		linphone_core_enable_video(theLinphoneCore, FALSE, FALSE);
	}

	LOGI(@"Linphone [%s]  started on [%s]", linphone_core_get_version(),
		 [[UIDevice currentDevice].model cStringUsingEncoding:[NSString defaultCStringEncoding]]);

	// Post event
	NSDictionary *dict = [NSDictionary dictionaryWithObject:[NSValue valueWithPointer:theLinphoneCore] forKey:@"core"];

	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneCoreUpdate
														object:[LinphoneManager instance]
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

	_iapManager = [[InAppProductsManager alloc] init];

	linphone_core_migrate_to_multi_transport(theLinphoneCore);

	// init audio session (just getting the instance will init)
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];
	BOOL bAudioInputAvailable = audioSession.inputAvailable;
	NSError *err;

	if (![audioSession setActive:NO error:&err] && err) {
		LOGE(@"audioSession setActive failed: %@", [err description]);
	}
	if (!bAudioInputAvailable) {
		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"No microphone", nil)
					  message:NSLocalizedString(
								  @"You need to plug a microphone to your device to use this application.", nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"Ok", nil)
			otherButtonTitles:nil, nil];
		[error show];
	}

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
		// go directly to bg mode
		[self enterBackgroundMode];
	}
}

- (void)createLinphoneCore {

	if (theLinphoneCore != nil) {
		LOGI(@"linphonecore is already created");
		return;
	}
	linphone_core_set_log_collection_path([[LinphoneManager cacheDirectory] UTF8String]);
	[self setLogsEnabled:[self lpConfigBoolForKey:@"debugenable_preference"]];
	LOGI(@"Create linphonecore");
	connectivity = none;

	ms_init(); // Need to initialize mediastreamer2 before loading the plugins
	// Load plugins if available in the linphone SDK - otherwise these calls will do nothing
	libmsilbc_init();
	libmssilk_init();
	libmsamr_init();
	libmsx264_init();
	libmsopenh264_init();
	libmsbcg729_init();
	libmswebrtc_init();

	// Set audio assets
	const char *lRing =
		[[LinphoneManager bundleFile:@"ring.wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	lp_config_set_string(configDb, "sound", "local_ring", lRing);
	const char *lRingBack =
		[[LinphoneManager bundleFile:@"ringback.wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	lp_config_set_string(configDb, "sound", "ringback_tone", lRingBack);
	const char *lPlay =
		[[LinphoneManager bundleFile:@"hold.wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	lp_config_set_string(configDb, "sound", "hold_music", lPlay);

	theLinphoneCore =
		linphone_core_new_with_config(&linphonec_vtable, configDb, (__bridge void *)(self) /* user_data */);

	/* set the CA file no matter what, since the remote provisioning could be hitting an HTTPS server */
	const char *lRootCa =
		[[LinphoneManager bundleFile:@"rootca.pem"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_root_ca(theLinphoneCore, lRootCa);
	linphone_core_set_user_certificates_path(theLinphoneCore, [[LinphoneManager cacheDirectory] UTF8String]);

	/* The core will call the linphone_iphone_configuring_status_changed callback when the remote provisioning is loaded
	 (or skipped).
	 Wait for this to finish the code configuration */

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(audioSessionInterrupted:)
												 name:AVAudioSessionInterruptionNotification
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(globalStateChangedNotificationHandler:)
												 name:kLinphoneGlobalStateUpdate
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(configuringStateChangedNotificationHandler:)
												 name:kLinphoneConfiguringStateUpdate
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(inappReady:) name:kIAPReady object:nil];

	/*call iterate once immediately in order to initiate background connections with sip server or remote provisioning
	 * grab, if any */
	linphone_core_iterate(theLinphoneCore);
	// start scheduler
	mIterateTimer =
		[NSTimer scheduledTimerWithTimeInterval:0.02 target:self selector:@selector(iterate) userInfo:nil repeats:YES];
}

- (void)destroyLinphoneCore {
	[mIterateTimer invalidate];
	// just in case
	[self removeCTCallCenterCb];

	[[NSNotificationCenter defaultCenter] removeObserver:self];

	if (theLinphoneCore != nil) { // just in case application terminate before linphone core initialization

		for (FileTransferDelegate *ftd in _fileTransferDelegates) {
			[ftd stopAndDestroy];
		}
		[_fileTransferDelegates removeAllObjects];

		LOGI(@"Destroy linphonecore");
		linphone_core_destroy(theLinphoneCore);
		theLinphoneCore = nil;
		ms_exit(); // Uninitialize mediastreamer2

		// Post event
		NSDictionary *dict =
			[NSDictionary dictionaryWithObject:[NSValue valueWithPointer:theLinphoneCore] forKey:@"core"];
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneCoreUpdate
															object:[LinphoneManager instance]
														  userInfo:dict];

		SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		if (proxyReachability)
			CFRelease(proxyReachability);
		proxyReachability = nil;
	}
	libStarted = FALSE;
}

- (void)resetLinphoneCore {
	[self destroyLinphoneCore];
	[self createLinphoneCore];

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

- (void)cancelLocalNotifTimerForCallId:(NSString *)callid {
	// first, make sure this callid is not already involved in a call
	MSList *calls = (MSList *)linphone_core_get_calls(theLinphoneCore);
	MSList *call = ms_list_find_custom(calls, (MSCompareFunc)comp_call_id, [callid UTF8String]);
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
	MSList *calls = (MSList *)linphone_core_get_calls(theLinphoneCore);
	MSList *call = ms_list_find_custom(calls, (MSCompareFunc)comp_call_id, [callid UTF8String]);
	if (call != NULL) {
		[self acceptCall:(LinphoneCall *)call->data];
		return;
	};
}

- (void)addPushCallId:(NSString *)callid {
	// first, make sure this callid is not already involved in a call
	MSList *calls = (MSList *)linphone_core_get_calls(theLinphoneCore);
	if (ms_list_find_custom(calls, (MSCompareFunc)comp_call_id, [callid UTF8String])) {
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
	AudioServicesPlaySystemSound([LinphoneManager instance].sounds.vibrate);
}

static int comp_call_state_paused(const LinphoneCall *call, const void *param) {
	return linphone_call_get_state(call) != LinphoneCallPaused;
}

- (void)startCallPausedLongRunningTask {
	pausedCallBgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
	  LOGW(@"Call cannot be paused any more, too late");
	  [[UIApplication sharedApplication] endBackgroundTask:pausedCallBgTask];
	}];
	LOGI(@"Long running task started, remaining [%g s] because at least one call is paused",
		 [[UIApplication sharedApplication] backgroundTimeRemaining]);
}
- (BOOL)enterBackgroundMode {
	LinphoneProxyConfig *proxyCfg;
	linphone_core_get_default_proxy(theLinphoneCore, &proxyCfg);
	BOOL shouldEnterBgMode = FALSE;

	// handle proxy config if any
	if (proxyCfg) {
		if ([[LinphoneManager instance] lpConfigBoolForKey:@"backgroundmode_preference"] ||
			[[LinphoneManager instance] lpConfigBoolForKey:@"pushnotification_preference"]) {

			// For registration register
			[self refreshRegisters];
		}

		if ([[LinphoneManager instance] lpConfigBoolForKey:@"backgroundmode_preference"]) {

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
								  // kick up network cnx, just in case
								  [self refreshRegisters];
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
	const MSList *callList = linphone_core_get_calls(theLinphoneCore);
	if (!currentCall // no active call
		&& callList // at least one call in a non active state
		&& ms_list_find_custom((MSList *)callList, (MSCompareFunc)comp_call_state_paused, NULL)) {
		[self startCallPausedLongRunningTask];
	}
	if (callList) {
		/*if at least one call exist, enter normal bg mode */
		shouldEnterBgMode = TRUE;
	}
	/*stop the video preview*/
	if (theLinphoneCore) {
		linphone_core_enable_video_preview(theLinphoneCore, FALSE);
		linphone_core_iterate(theLinphoneCore);
	}
	linphone_core_stop_dtmf_stream(theLinphoneCore);

	LOGI(@"Entering [%s] bg mode", shouldEnterBgMode ? "normal" : "lite");

	if (!shouldEnterBgMode) {
		if ([[LinphoneManager instance] lpConfigBoolForKey:@"pushnotification_preference"]) {
			LOGI(@"Keeping lc core to handle push");
			/*destroy voip socket if any and reset connectivity mode*/
			connectivity = none;
			linphone_core_set_network_reachable(theLinphoneCore, FALSE);
			return YES;
		}
		return NO;

	} else
		return YES;
}

- (void)becomeActive {
	[self refreshRegisters];
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

	/*start the video preview in case we are in the main view*/
	if ([LinphoneManager runningOnIpad] && linphone_core_video_enabled(theLinphoneCore) &&
		[self lpConfigBoolForKey:@"preview_preference"]) {
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
}

- (void)beginInterruption {
	LinphoneCall *c = linphone_core_get_current_call(theLinphoneCore);
	LOGI(@"Sound interruption detected!");
	if (c && linphone_call_get_state(c) == LinphoneCallStreamsRunning) {
		linphone_core_pause_call(theLinphoneCore, c);
	}
}

- (void)endInterruption {
	LOGI(@"Sound interruption ended!");
}

- (void)refreshRegisters {
	if (connectivity == none) {
		// don't trust ios when he says there is no network. Create a new reachability context, the previous one might
		// be mis-functionning.
		[self setupNetworkReachabilityCallback];
	}
	linphone_core_refresh_registers(theLinphoneCore); // just to make sure REGISTRATION is up to date
}

- (void)renameDefaultSettings {
	// rename .linphonerc to linphonerc to ease debugging: when downloading
	// containers from MacOSX, Finder do not display hidden files leading
	// to useless painful operations to display the .linphonerc file
	NSString *src = [LinphoneManager documentFile:@".linphonerc"];
	NSString *dst = [LinphoneManager documentFile:@"linphonerc"];
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
	if ([LinphoneManager runningOnIpad] && [[NSFileManager defaultManager] fileExistsAtPath:srcIpad]) {
		src = srcIpad;
	}
	NSString *dst = [LinphoneManager documentFile:@"linphonerc"];
	[LinphoneManager copyFile:src destination:dst override:FALSE];
}

- (void)overrideDefaultSettings {
	NSString *factory = [LinphoneManager bundleFile:@"linphonerc-factory"];
	NSString *factoryIpad = [LinphoneManager bundleFile:@"linphonerc-factory~ipad"];
	if ([LinphoneManager runningOnIpad] && [[NSFileManager defaultManager] fileExistsAtPath:factoryIpad]) {
		factory = factoryIpad;
	}
	NSString *confiFileName = [LinphoneManager documentFile:@"linphonerc"];
	configDb = lp_config_new_with_factory([confiFileName cStringUsingEncoding:[NSString defaultCStringEncoding]],
										  [factory cStringUsingEncoding:[NSString defaultCStringEncoding]]);
}
#pragma mark - Audio route Functions

- (bool)allowSpeaker {
	bool notallow = false;
	CFStringRef lNewRoute = CFSTR("Unknown");
	UInt32 lNewRouteSize = sizeof(lNewRoute);
	OSStatus lStatus = AudioSessionGetProperty(kAudioSessionProperty_AudioRoute, &lNewRouteSize, &lNewRoute);
	if (!lStatus && lNewRouteSize > 0) {
		NSString *route = (__bridge NSString *)lNewRoute;
		notallow = [route isEqualToString:@"Headset"] || [route isEqualToString:@"Headphone"] ||
				   [route isEqualToString:@"HeadphonesAndMicrophone"] || [route isEqualToString:@"HeadsetInOut"] ||
				   [route isEqualToString:@"Lineout"] || [LinphoneManager runningOnIpad];
		CFRelease(lNewRoute);
	}
	return !notallow;
}

static void audioRouteChangeListenerCallback(void *inUserData,					  // 1
											 AudioSessionPropertyID inPropertyID, // 2
											 UInt32 inPropertyValueSize,		  // 3
											 const void *inPropertyValue		  // 4
											 ) {
	if (inPropertyID != kAudioSessionProperty_AudioRouteChange)
		return; // 5
	LinphoneManager *lm = (__bridge LinphoneManager *)inUserData;

	bool speakerEnabled = false;
	CFStringRef lNewRoute = CFSTR("Unknown");
	UInt32 lNewRouteSize = sizeof(lNewRoute);
	OSStatus lStatus = AudioSessionGetProperty(kAudioSessionProperty_AudioRoute, &lNewRouteSize, &lNewRoute);
	if (!lStatus && lNewRouteSize > 0) {
		NSString *route = (__bridge NSString *)lNewRoute;
		LOGI(@"Current audio route is [%s]", [route cStringUsingEncoding:[NSString defaultCStringEncoding]]);

		speakerEnabled = [route isEqualToString:@"Speaker"] || [route isEqualToString:@"SpeakerAndMicrophone"];
		if (![LinphoneManager runningOnIpad] && [route isEqualToString:@"HeadsetBT"] && !speakerEnabled) {
			lm.bluetoothEnabled = TRUE;
			lm.bluetoothAvailable = TRUE;
			NSDictionary *dict = [NSDictionary
				dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:lm.bluetoothAvailable], @"available", nil];
			[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneBluetoothAvailabilityUpdate
																object:lm
															  userInfo:dict];
		} else {
			lm.bluetoothEnabled = FALSE;
		}
		CFRelease(lNewRoute);
	}

	if (speakerEnabled != lm.speakerEnabled) { // Reforce value
		lm.speakerEnabled = lm.speakerEnabled;
	}
}

- (void)setSpeakerEnabled:(BOOL)enable {
	speakerEnabled = enable;

	if (enable && [self allowSpeaker]) {
		UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
		AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(audioRouteOverride),
								&audioRouteOverride);
		bluetoothEnabled = FALSE;
	} else {
		UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;
		AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(audioRouteOverride),
								&audioRouteOverride);
	}

	if (bluetoothAvailable) {
		UInt32 bluetoothInputOverride = bluetoothEnabled;
		AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryEnableBluetoothInput,
								sizeof(bluetoothInputOverride), &bluetoothInputOverride);
	}
}

- (void)setBluetoothEnabled:(BOOL)enable {
	if (bluetoothAvailable) {
		// The change of route will be done in setSpeakerEnabled
		bluetoothEnabled = enable;
		if (bluetoothEnabled) {
			[self setSpeakerEnabled:FALSE];
		} else {
			[self setSpeakerEnabled:speakerEnabled];
		}
	}
}

#pragma mark - Call Functions

- (void)acceptCall:(LinphoneCall *)call {
	LinphoneCallParams *lcallParams = linphone_core_create_call_params(theLinphoneCore, call);
	if ([self lpConfigBoolForKey:@"edge_opt_preference"]) {
		bool low_bandwidth = self.network == network_2g;
		if (low_bandwidth) {
			LOGI(@"Low bandwidth mode");
		}
		linphone_call_params_enable_low_bandwidth(lcallParams, low_bandwidth);
	}

	linphone_core_accept_call_with_params(theLinphoneCore, call, lcallParams);
}

- (void)call:(NSString *)address displayName:(NSString *)displayName transfer:(BOOL)transfer {
	// First verify that network is available, abort otherwise.
	if (!linphone_core_is_network_reachable(theLinphoneCore)) {
		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Network Error", nil)
					  message:
						  NSLocalizedString(
							  @"There is no network connection available, enable WIFI or WWAN prior to place a call",
							  nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
			otherButtonTitles:nil];
		[error show];
		return;
	}

	// Then check that no GSM calls are in progress, abort otherwise.
	CTCallCenter *callCenter = [[CTCallCenter alloc] init];
	if ([callCenter currentCalls] != nil) {
		LOGE(@"GSM call in progress, cancelling outgoing SIP call request");
		UIAlertView *error =
			[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Cannot make call", nil)
									   message:NSLocalizedString(@"Please terminate GSM call first.", nil)
									  delegate:nil
							 cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
							 otherButtonTitles:nil];
		[error show];
		return;
	}

	LinphoneAddress *addr = NULL;
	// Continue by checking that the provided address is a valid SIP address, abort otherwise.
	if ([address length] == 0) {
		// no address provided... nothing to do
	} else if (![address canBeConvertedToEncoding:[NSString defaultCStringEncoding]]) {
		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Invalid SIP address", nil)
					  message:NSLocalizedString(
								  @"Some invalid characters where found in the given SIP address. Please correct it.",
								  nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
			otherButtonTitles:nil];
		[error show];
	} else if ((addr = linphone_core_interpret_url(
					theLinphoneCore, [address cStringUsingEncoding:[NSString defaultCStringEncoding]])) == NULL) {
		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Invalid SIP address", nil)
					  message:NSLocalizedString(@"Either configure a SIP proxy server from settings prior to place a "
												@"call or use a valid SIP address (I.E sip:john@example.net)",
												nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
			otherButtonTitles:nil];
		[error show];
	} else {
		// Finally we can make the call
		LinphoneProxyConfig *proxyCfg;
		linphone_core_get_default_proxy(theLinphoneCore, &proxyCfg);
		LinphoneCallParams *lcallParams = linphone_core_create_default_call_parameters(theLinphoneCore);
		if ([self lpConfigBoolForKey:@"edge_opt_preference"] && (self.network == network_2g)) {
			LOGI(@"Enabling low bandwidth mode");
			linphone_call_params_enable_low_bandwidth(lcallParams, YES);
		}
		if (displayName != nil) {
			linphone_address_set_display_name(addr,
											  [displayName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}
		if ([[LinphoneManager instance] lpConfigBoolForKey:@"override_domain_with_default_one"]) {
			linphone_address_set_domain(
				addr, [[[LinphoneManager instance] lpConfigStringForKey:@"domain" forSection:@"wizard"]
						  cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}

		if (transfer) {
			char *caddr = linphone_address_as_string(addr);
			linphone_core_transfer_call(theLinphoneCore, linphone_core_get_current_call(theLinphoneCore), caddr);
			ms_free(caddr);
		} else {
			LinphoneCall *call = linphone_core_invite_address_with_params(theLinphoneCore, addr, lcallParams);
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
	}
}

#pragma mark - Property Functions

- (void)setPushNotificationToken:(NSData *)apushNotificationToken {
	if (apushNotificationToken == pushNotificationToken) {
		return;
	}
	if (pushNotificationToken != nil) {
		pushNotificationToken = nil;
	}

	if (apushNotificationToken != nil) {
		pushNotificationToken = apushNotificationToken;
	}
	LinphoneProxyConfig *cfg = nil;
	linphone_core_get_default_proxy(theLinphoneCore, &cfg);
	if (cfg) {
		linphone_proxy_config_edit(cfg);
		[self configurePushTokenForProxyConfig:cfg];
		linphone_proxy_config_done(cfg);
	}
}

- (void)configurePushTokenForProxyConfig:(LinphoneProxyConfig *)proxyCfg {
	NSData *tokenData = pushNotificationToken;
	if (tokenData != nil && [self lpConfigBoolForKey:@"pushnotification_preference"]) {
		const unsigned char *tokenBuffer = [tokenData bytes];
		NSMutableString *tokenString = [NSMutableString stringWithCapacity:[tokenData length] * 2];
		for (int i = 0; i < [tokenData length]; ++i) {
			[tokenString appendFormat:@"%02X", (unsigned int)tokenBuffer[i]];
		}
// NSLocalizedString(@"IC_MSG", nil); // Fake for genstrings
// NSLocalizedString(@"IM_MSG", nil); // Fake for genstrings
#ifdef USE_APN_DEV
#define APPMODE_SUFFIX @"dev"
#else
#define APPMODE_SUFFIX @"prod"
#endif
		NSString *params =
			[NSString stringWithFormat:@"app-id=%@.%@;pn-type=apple;pn-tok=%@;pn-msg-str=IM_MSG;pn-call-str=IC_MSG;pn-"
									   @"call-snd=ring.caf;pn-msg-snd=msg.caf",
									   [[NSBundle mainBundle] bundleIdentifier], APPMODE_SUFFIX, tokenString];

		linphone_proxy_config_set_contact_uri_parameters(proxyCfg, [params UTF8String]);
		linphone_proxy_config_set_contact_parameters(proxyCfg, NULL);
	} else {
		// no push token:
		linphone_proxy_config_set_contact_uri_parameters(proxyCfg, NULL);
		linphone_proxy_config_set_contact_parameters(proxyCfg, NULL);
	}
}

#pragma mark - Misc Functions

+ (NSString *)bundleFile:(NSString *)file {
	return [[NSBundle mainBundle] pathForResource:[file stringByDeletingPathExtension] ofType:[file pathExtension]];
}

+ (NSString *)documentFile:(NSString *)file {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsPath = [paths objectAtIndex:0];
	return [documentsPath stringByAppendingPathComponent:file];
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
	const MSList *rooms = linphone_core_get_chat_rooms([LinphoneManager getLc]);
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
	NSFileManager *fileManager = [NSFileManager defaultManager];
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
		configDb, "audio", "codec_bitrate_limit",
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

- (void)setLogsEnabled:(BOOL)enabled {
	if ([LinphoneManager isRunningTests]) {
		NSLog(@"Running tests, forcing logs to MESSAGE level");
		linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
		linphone_core_set_log_level(ORTP_MESSAGE);
	} else {
		if (enabled) {
			NSLog(@"Enabling debug logs");
			linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
			linphone_core_set_log_level(ORTP_DEBUG);
			linphone_core_enable_log_collection(enabled);
		} else {
			NSLog(@"Disabling debug logs");
			linphone_core_enable_log_collection(enabled);
			linphone_core_disable_logs();
		}
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
	[self lpConfigSetString:value forKey:key forSection:LINPHONERC_APPLICATION_KEY];
}
- (void)lpConfigSetString:(NSString *)value forKey:(NSString *)key forSection:(NSString *)section {
	if (!key)
		return;
	lp_config_set_string(configDb, [section UTF8String], [key UTF8String], value ? [value UTF8String] : NULL);
}
- (NSString *)lpConfigStringForKey:(NSString *)key {
	return [self lpConfigStringForKey:key withDefault:nil];
}
- (NSString *)lpConfigStringForKey:(NSString *)key withDefault:(NSString *)defaultValue {
	return [self lpConfigStringForKey:key forSection:LINPHONERC_APPLICATION_KEY withDefault:defaultValue];
}
- (NSString *)lpConfigStringForKey:(NSString *)key forSection:(NSString *)section {
	return [self lpConfigStringForKey:key forSection:section withDefault:nil];
}
- (NSString *)lpConfigStringForKey:(NSString *)key forSection:(NSString *)section withDefault:(NSString *)defaultValue {
	if (!key)
		return defaultValue;
	const char *value = lp_config_get_string(configDb, [section UTF8String], [key UTF8String], NULL);
	return value ? [NSString stringWithUTF8String:value] : defaultValue;
}

- (void)lpConfigSetInt:(int)value forKey:(NSString *)key {
	[self lpConfigSetInt:value forKey:key forSection:LINPHONERC_APPLICATION_KEY];
}
- (void)lpConfigSetInt:(int)value forKey:(NSString *)key forSection:(NSString *)section {
	if (!key)
		return;
	lp_config_set_int(configDb, [section UTF8String], [key UTF8String], (int)value);
}
- (int)lpConfigIntForKey:(NSString *)key {
	return [self lpConfigIntForKey:key withDefault:-1];
}
- (int)lpConfigIntForKey:(NSString *)key withDefault:(int)defaultValue {
	return [self lpConfigIntForKey:key forSection:LINPHONERC_APPLICATION_KEY withDefault:defaultValue];
}
- (int)lpConfigIntForKey:(NSString *)key forSection:(NSString *)section {
	return [self lpConfigIntForKey:key forSection:section withDefault:-1];
}
- (int)lpConfigIntForKey:(NSString *)key forSection:(NSString *)section withDefault:(int)defaultValue {
	if (!key)
		return defaultValue;
	return lp_config_get_int(configDb, [section UTF8String], [key UTF8String], (int)defaultValue);
}

- (void)lpConfigSetBool:(BOOL)value forKey:(NSString *)key {
	[self lpConfigSetBool:value forKey:key forSection:LINPHONERC_APPLICATION_KEY];
}
- (void)lpConfigSetBool:(BOOL)value forKey:(NSString *)key forSection:(NSString *)section {
	[self lpConfigSetInt:(int)(value == TRUE)forKey:key forSection:section];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key {
	return [self lpConfigBoolForKey:key withDefault:FALSE];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key withDefault:(BOOL)defaultValue {
	return [self lpConfigBoolForKey:key forSection:LINPHONERC_APPLICATION_KEY withDefault:defaultValue];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key forSection:(NSString *)section {
	return [self lpConfigBoolForKey:key forSection:section withDefault:FALSE];
}
- (BOOL)lpConfigBoolForKey:(NSString *)key forSection:(NSString *)section withDefault:(BOOL)defaultValue {
	if (!key)
		return defaultValue;
	int val = [self lpConfigIntForKey:key forSection:section withDefault:-1];
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
	CTCallCenter *ct = (CTCallCenter *)cCenter;
	/* pause current call, if any */
	LinphoneCall *call = linphone_core_get_current_call(theLinphoneCore);
	if ([ct currentCalls] != nil) {
		if (call) {
			LOGI(@"Pausing SIP call because GSM call");
			linphone_core_pause_call(theLinphoneCore, call);
			[self startCallPausedLongRunningTask];
		} else if (linphone_core_is_in_conference(theLinphoneCore)) {
			LOGI(@"Leaving conference call because GSM call");
			linphone_core_leave_conference(theLinphoneCore);
			[self startCallPausedLongRunningTask];
		}
	} // else nop, keep call in paused state
}

- (NSString *)contactFilter {
	NSString *filter = @"*";
	if ([self lpConfigBoolForKey:@"contact_filter_on_default_domain"]) {
		LinphoneProxyConfig *proxy_cfg;
		linphone_core_get_default_proxy(theLinphoneCore, &proxy_cfg);
		if (proxy_cfg && linphone_proxy_config_get_addr(proxy_cfg)) {
			return [NSString stringWithCString:linphone_proxy_config_get_domain(proxy_cfg)
									  encoding:[NSString defaultCStringEncoding]];
		}
	}
	return filter;
}

#pragma Tunnel

- (void)setTunnelMode:(TunnelMode)atunnelMode {
	LinphoneTunnel *tunnel = linphone_core_get_tunnel(theLinphoneCore);
	tunnelMode = atunnelMode;
	switch (tunnelMode) {
	case tunnel_off:
		linphone_tunnel_enable(tunnel, false);
		break;
	case tunnel_on:
		linphone_tunnel_enable(tunnel, true);
		break;
	case tunnel_wwan:
		if (connectivity != wwan) {
			linphone_tunnel_enable(tunnel, false);
		} else {
			linphone_tunnel_enable(tunnel, true);
		}
		break;
	case tunnel_auto:
		linphone_tunnel_auto_detect(tunnel);
		break;
	}
}

#pragma mark - InApp Purchase events

- (void)inappReady:(NSNotification *)notif {
	// Query our in-app server to retrieve InApp purchases
	[_iapManager retrievePurchases];
}

- (void)removeAllAccounts {
	linphone_core_clear_proxy_config([LinphoneManager getLc]);
	linphone_core_clear_all_auth_info([LinphoneManager getLc]);
}

@end
