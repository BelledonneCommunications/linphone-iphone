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

#import "LinphoneManager.h"
#import "LinphoneCoreSettingsStore.h"
#import "ChatModel.h"

#include "linphonecore_utils.h"
#include "lpconfig.h"
#include "private.h"

static LinphoneCore* theLinphoneCore=nil;
static LinphoneManager* theLinphoneManager=nil;
const NSString *CONTACT_SIP_FIELD = @"SIP";

extern void libmsilbc_init();
#ifdef HAVE_AMR
extern void libmsamr_init();
#endif

#ifdef HAVE_X264
extern void libmsx264_init();
#endif
#define FRONT_CAM_NAME "AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:1" /*"AV Capture: Front Camera"*/
#define BACK_CAM_NAME "AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:0" /*"AV Capture: Back Camera"*/

#if defined (HAVE_SILK)
extern void libmssilk_init(); 
#endif

#if HAVE_G729
extern  void libmsbcg729_init();
#endif

@implementation LinphoneManager

@synthesize connectivity;
@synthesize frontCamId;
@synthesize backCamId;
@synthesize defaultExpires;
@synthesize settingsStore;
@synthesize database;
@synthesize fastAddressBook;
@synthesize pushNotificationToken;

struct codec_name_pref_table{
    const char *name;
    int rate;
    NSString *prefname;
};

struct codec_name_pref_table codec_pref_table[]={
	{ "speex", 8000, @"speex_8k_preference" },
	{ "speex", 16000, @"speex_16k_preference" },
	{ "silk", 24000, @"silk_24k_preference" },
	{ "silk", 16000, @"silk_16k_preference" },
	{ "amr", 8000, @"amr_8k_preference" },
	{ "ilbc", 8000, @"ilbc_preference"},
	{ "pcmu", 8000, @"pcmu_preference"},
	{ "pcma", 8000, @"pcma_preference"},
	{ "g722", 8000, @"g722_preference"},
	{ "g729", 8000, @"g729_preference"},
	{ "mp4v-es", 90000, @"mp4v-es_preference"},
	{ "h264", 90000, @"h264_preference"},
	{ "vp8", 90000, @"vp8_preference"},
	{ NULL,0,Nil }
};

+ (NSString *)getPreferenceForCodec: (const char*) name withRate: (int) rate{
	int i;
	for(i=0;codec_pref_table[i].name!=NULL;++i){
		if (strcasecmp(codec_pref_table[i].name,name)==0 && codec_pref_table[i].rate==rate)
			return codec_pref_table[i].prefname;
	}
	return Nil;
}

+ (NSSet *)unsupportedCodecs {
    NSMutableSet *set = [NSMutableSet set];
	for(int i=0;codec_pref_table[i].name!=NULL;++i) {
        if(linphone_core_find_payload_type([LinphoneManager getLc],codec_pref_table[i].name, codec_pref_table[i].rate) == NULL) {
            [set addObject:codec_pref_table[i].prefname];
		}
	}
	return set;
}

+ (BOOL)runningOnIpad {
#ifdef UI_USER_INTERFACE_IDIOM
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
#else
    return NO;
#endif
}

+ (BOOL)isNotIphone3G
{
	static BOOL done=FALSE;
	static BOOL result;
	if (!done){
		size_t size;
		sysctlbyname("hw.machine", NULL, &size, NULL, 0);
		char *machine = malloc(size);
		sysctlbyname("hw.machine", machine, &size, NULL, 0);
		NSString *platform = [[NSString alloc ] initWithUTF8String:machine];
		free(machine);
        
		result = ![platform isEqualToString:@"iPhone1,2"];
        
		[platform release];
		done=TRUE;
	}
    return result;
}

+ (NSString *)getUserAgent {
    return [NSString stringWithFormat:@"LinphoneIphone/%@ (Linphone/%s; Apple %@/%@)",
            [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString*)kCFBundleVersionKey],
            linphone_core_get_version(),
            [UIDevice currentDevice].systemName,
            [UIDevice currentDevice].systemVersion];
}

+ (LinphoneManager*)instance {
    if(theLinphoneManager == nil) {
        theLinphoneManager = [LinphoneManager alloc];
        [theLinphoneManager init];
    }
	return theLinphoneManager;
}


#pragma mark - Lifecycle Functions

- (id)init {
    if ((self = [super init])) {
        fastAddressBook = [[FastAddressBook alloc] init];
        database = NULL;
        settingsStore = nil;
		self.defaultExpires = 600;
        [self openDatabase];
    }
    return self;
}

- (void)dealloc {
    [fastAddressBook release];
    [self closeDatabase];
    [settingsStore release];
    
    [super dealloc];
}


#pragma mark - Database Functions

- (void)openDatabase {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsPath = [paths objectAtIndex:0];
    NSString *databaseDocumentPath = [documentsPath stringByAppendingPathComponent:@"database.txt"];

    // Copy default database
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error = nil;
    //[fileManager removeItemAtPath:databaseDocumentPath error:&error]; //TODO REMOVE
    if ([fileManager fileExistsAtPath:databaseDocumentPath] == NO) {
        [LinphoneLogger logc:LinphoneLoggerLog format:"Create sqlite3 database"];
        NSString *resourceDocumentPath = [[NSBundle mainBundle] pathForResource:@"database" ofType:@"sqlite"];
        [fileManager copyItemAtPath:resourceDocumentPath toPath:databaseDocumentPath error:&error];
        if(error != nil) {
            [LinphoneLogger log:LinphoneLoggerError format:@"Can't copy database: %@", [error localizedDescription]];
            return;
        }
    }

    if(sqlite3_open([databaseDocumentPath UTF8String], &database) != SQLITE_OK) {
        [LinphoneLogger log:LinphoneLoggerError format:@"Can't open \"%@\" sqlite3 database.", databaseDocumentPath];
    }
}

- (void)closeDatabase {
    if(database != NULL) {
        if(sqlite3_close(database) != SQLITE_OK) {
            [LinphoneLogger logc:LinphoneLoggerError format:"Can't close sqlite3 database."];
        }
    }
}


#pragma mark - Linphone Core Functions

+ (LinphoneCore*)getLc {
	if (theLinphoneCore==nil) {
		@throw([NSException exceptionWithName:@"LinphoneCoreException" reason:@"Linphone core not initialized yet" userInfo:nil]);
	}
	return theLinphoneCore;
}

+ (BOOL)isLcReady {
    return theLinphoneCore != nil;
}


#pragma mark - Logs Functions

//generic log handler for debug version
void linphone_iphone_log_handler(int lev, const char *fmt, va_list args){
	NSString* format = [[NSString alloc] initWithUTF8String:fmt];
	NSLogv(format, args);
	NSString* formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	//[[LinphoneManager instance] addLog:formatedString];
	[formatedString release];
    [format release];
}

//Error/warning log handler 
static void linphone_iphone_log(struct _LinphoneCore * lc, const char * message) {
	NSString* log = [NSString stringWithCString:message encoding:[NSString defaultCStringEncoding]]; 
	NSLog(log, NULL);
	//[[LinphoneManager instance] addLog:log];
}


#pragma mark - Display Status Functions

- (void)displayStatus:(NSString*) message {
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                           message, @"message", 
                           nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneDisplayStatus" object:self userInfo:dict];
}


static void linphone_iphone_display_status(struct _LinphoneCore * lc, const char * message) {
    NSString* status = [[NSString alloc] initWithCString:message encoding:[NSString defaultCStringEncoding]];
	[(LinphoneManager*)linphone_core_get_user_data(lc)  displayStatus:status];
    [status release];
}


#pragma mark - Call State Functions

- (void)onCall:(LinphoneCall*)call StateChanged:(LinphoneCallState)state withMessage:(const char *)message {
    // Handling wrapper
    if(state == LinphoneCallReleased) {
        if(linphone_call_get_user_pointer(call) != NULL) {
            free (linphone_call_get_user_pointer(call));
            linphone_call_set_user_pointer(call, NULL);
        }
        return;
    }
    if (!linphone_call_get_user_pointer(call)) {
        LinphoneCallAppData* data = (LinphoneCallAppData*) malloc(sizeof(LinphoneCallAppData));
        data->batteryWarningShown = FALSE;
        data->notification = nil;
        linphone_call_set_user_pointer(call, data);
    }
    
    // Disable speaker when no more call
    if ((state == LinphoneCallEnd || state == LinphoneCallError)) {
        if(linphone_core_get_calls_nb([LinphoneManager getLc]) == 0)
            [self enableSpeaker:FALSE];
    }
    
    // Enable speaker when video
    if(state == LinphoneCallIncomingReceived ||
       state == LinphoneCallOutgoingInit ||
       state == LinphoneCallConnected ||
       state == LinphoneCallStreamsRunning ||
       state == LinphoneCallUpdated) {
        if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
            [self enableSpeaker:TRUE];
        }
    }
    
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                           [NSValue valueWithPointer:call], @"call",
                           [NSNumber numberWithInt:state], @"state", 
                           [NSString stringWithUTF8String:message], @"message", nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneCallUpdate" object:self userInfo:dict];
}

static void linphone_iphone_call_state(LinphoneCore *lc, LinphoneCall* call, LinphoneCallState state,const char* message) {
	[(LinphoneManager*)linphone_core_get_user_data(lc) onCall:call StateChanged: state withMessage:  message];
}


#pragma mark - Transfert State Functions

static void linphone_iphone_transfer_state_changed(LinphoneCore* lc, LinphoneCall* call, LinphoneCallState state) {
}


#pragma mark - Registration State Functions

-(void) onRegister:(LinphoneCore *)lc cfg:(LinphoneProxyConfig*) cfg state:(LinphoneRegistrationState) state message:(const char*) message {
    [LinphoneLogger logc:LinphoneLoggerLog format:"NEW REGISTRATION STATE: '%s' (message: '%s')", linphone_registration_state_to_string(state), message];
    
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                          [NSNumber numberWithInt:state], @"state", 
                          [NSValue valueWithPointer:cfg], @"cfg",
                          [NSString stringWithUTF8String:message], @"message", 
                          nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneRegistrationUpdate" object:self userInfo:dict];
}

static void linphone_iphone_registration_state(LinphoneCore *lc, LinphoneProxyConfig* cfg, LinphoneRegistrationState state,const char* message) {
	[(LinphoneManager*)linphone_core_get_user_data(lc) onRegister:lc cfg:cfg state:state message:message];
}


#pragma mark - Text Received Functions

- (void)onTextReceived:(LinphoneCore *)lc room:(LinphoneChatRoom *)room from:(const LinphoneAddress *)from message:(const char *)message {
    
    char *fromStr = linphone_address_as_string_uri_only(from);
    if(fromStr == NULL)
        return;
    
    // Save message in database
    ChatModel *chat = [[ChatModel alloc] init];
    [chat setLocalContact:@""];
    [chat setRemoteContact:[NSString stringWithUTF8String:fromStr]];
    [chat setMessage:[NSString stringWithUTF8String:message]];
    [chat setDirection:[NSNumber numberWithInt:1]];
    [chat setTime:[NSDate date]];
    [chat setRead:[NSNumber numberWithInt:0]];
    [chat create];
    
    ms_free(fromStr);
    
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                           [NSValue valueWithPointer:room], @"room", 
                           [NSValue valueWithPointer:from], @"from",
                           [NSString stringWithUTF8String:message], @"message", 
                           chat, @"chat", 
                           nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneTextReceived" object:self userInfo:dict]; 
    [chat release];
}

static void linphone_iphone_text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message) {
    [(LinphoneManager*)linphone_core_get_user_data(lc) onTextReceived:lc room:room from:from message:message];
}


#pragma mark - Network Functions

- (void)kickOffNetworkConnection {
	/*start a new thread to avoid blocking the main ui in case of peer host failure*/
	[NSThread detachNewThreadSelector:@selector(runNetworkConnection) toTarget:self withObject:nil];
}

- (void)runNetworkConnection {
	CFWriteStreamRef writeStream;
	CFStreamCreatePairWithSocketToHost(NULL, (CFStringRef)@"192.168.0.200"/*"linphone.org"*/, 15000, nil, &writeStream);
	CFWriteStreamOpen (writeStream);
	const char* buff="hello";
	CFWriteStreamWrite (writeStream,(const UInt8*)buff,strlen(buff));
	CFWriteStreamClose (writeStream);
}	

static void showNetworkFlags(SCNetworkReachabilityFlags flags){
	[LinphoneLogger logc:LinphoneLoggerLog format:"Network connection flags:"];
	if (flags==0) [LinphoneLogger logc:LinphoneLoggerLog format:"no flags."];
	if (flags & kSCNetworkReachabilityFlagsTransientConnection)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsTransientConnection"];
	if (flags & kSCNetworkReachabilityFlagsReachable)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsReachable"];
	if (flags & kSCNetworkReachabilityFlagsConnectionRequired)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsConnectionRequired"];
	if (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsConnectionOnTraffic"];
	if (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsConnectionOnDemand"];
	if (flags & kSCNetworkReachabilityFlagsIsLocalAddress)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsIsLocalAddress"];
	if (flags & kSCNetworkReachabilityFlagsIsDirect)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsIsDirect"];
	if (flags & kSCNetworkReachabilityFlagsIsWWAN)
		[LinphoneLogger logc:LinphoneLoggerLog format:"kSCNetworkReachabilityFlagsIsWWAN"];
}

void networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* nilCtx){
	showNetworkFlags(flags);
	LinphoneManager* lLinphoneMgr = [LinphoneManager instance];
	SCNetworkReachabilityFlags networkDownFlags=kSCNetworkReachabilityFlagsConnectionRequired |kSCNetworkReachabilityFlagsConnectionOnTraffic | kSCNetworkReachabilityFlagsConnectionOnDemand;

	if ([LinphoneManager getLc] != nil) {
		LinphoneProxyConfig* proxy;
		linphone_core_get_default_proxy([LinphoneManager getLc], &proxy);

        struct NetworkReachabilityContext* ctx = nilCtx ? ((struct NetworkReachabilityContext*)nilCtx) : 0;
		if ((flags == 0) || (flags & networkDownFlags)) {
			linphone_core_set_network_reachable([LinphoneManager getLc],false);
			lLinphoneMgr.connectivity = none;
			[[LinphoneManager instance] kickOffNetworkConnection];
		} else {
			Connectivity  newConnectivity;
			BOOL isWifiOnly = lp_config_get_int(linphone_core_get_config([LinphoneManager getLc]),"app","wifi_only_preference",FALSE);
            if (!ctx || ctx->testWWan)
                newConnectivity = flags & kSCNetworkReachabilityFlagsIsWWAN ? wwan:wifi;
            else
                newConnectivity = wifi;

			if (newConnectivity == wwan 
				&& proxy 
				&& isWifiOnly 
				&& (lLinphoneMgr.connectivity == newConnectivity || lLinphoneMgr.connectivity == none)) {
				linphone_proxy_config_expires(proxy, 0);
			} else if (proxy){
				linphone_proxy_config_expires(proxy, lLinphoneMgr.defaultExpires); //might be better to save the previous value
			}
			
			if (lLinphoneMgr.connectivity == none) {
				linphone_core_set_network_reachable([LinphoneManager getLc],true);
				
			} else if (lLinphoneMgr.connectivity != newConnectivity) {
				// connectivity has changed
				linphone_core_set_network_reachable([LinphoneManager getLc],false);
				if (newConnectivity == wwan && proxy && isWifiOnly) {
					linphone_proxy_config_expires(proxy, 0);
				} 
				linphone_core_set_network_reachable([LinphoneManager getLc],true);
				[LinphoneLogger logc:LinphoneLoggerLog format:"Network connectivity changed to type [%s]",(newConnectivity==wifi?"wifi":"wwan")];
			}
			lLinphoneMgr.connectivity=newConnectivity;
		}
		if (ctx && ctx->networkStateChanged) {
            (*ctx->networkStateChanged)(lLinphoneMgr.connectivity);
        }
	}
}

- (void)setupNetworkReachabilityCallback {
	SCNetworkReachabilityContext *ctx=NULL;
	const char *nodeName="linphone.org";
	
    if (proxyReachability) {
        [LinphoneLogger logc:LinphoneLoggerLog format:"Cancelling old network reachability"];
        SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(proxyReachability);
        proxyReachability = nil;
    }
    
    proxyReachability = SCNetworkReachabilityCreateWithName(nil, nodeName);
    
	//initial state is network off should be done as soon as possible
	SCNetworkReachabilityFlags flags;
	if (!SCNetworkReachabilityGetFlags(proxyReachability, &flags)) {
		[LinphoneLogger logc:LinphoneLoggerError format:"Cannot get reachability flags: %s", SCErrorString(SCError())];
		return;
	}
	networkReachabilityCallBack(proxyReachability, flags, ctx ? ctx->info : 0);

	if (!SCNetworkReachabilitySetCallback(proxyReachability, (SCNetworkReachabilityCallBack)networkReachabilityCallBack, ctx)){
		[LinphoneLogger logc:LinphoneLoggerError format:"Cannot register reachability cb: %s", SCErrorString(SCError())];
		return;
	}
	if(!SCNetworkReachabilityScheduleWithRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode)){
		[LinphoneLogger logc:LinphoneLoggerError format:"Cannot register schedule reachability cb: %s", SCErrorString(SCError())];
		return;
	}
}


#pragma mark - 

static LinphoneCoreVTable linphonec_vtable = {
	.show =NULL,
	.call_state_changed =(LinphoneCallStateCb)linphone_iphone_call_state,
	.registration_state_changed = linphone_iphone_registration_state,
	.notify_recv = NULL,
	.new_subscription_request = NULL,
	.auth_info_requested = NULL,
	.display_status = linphone_iphone_display_status,
	.display_message=linphone_iphone_log,
	.display_warning=linphone_iphone_log,
	.display_url=NULL,
	.text_received=linphone_iphone_text_received,
	.dtmf_received=NULL,
    .transfer_state_changed=linphone_iphone_transfer_state_changed
};

//scheduling loop
- (void)iterate {
	linphone_core_iterate(theLinphoneCore);
}

- (void)startLibLinphone {
	
	//get default config from bundle
	NSBundle* myBundle = [NSBundle mainBundle];
	NSString* factoryConfig = [myBundle pathForResource:[LinphoneManager runningOnIpad]?@"linphonerc-ipad":@"linphonerc" ofType:nil] ;
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *confiFileName = [[paths objectAtIndex:0] stringByAppendingString:@"/.linphonerc"];
	NSString *zrtpSecretsFileName = [[paths objectAtIndex:0] stringByAppendingString:@"/zrtp_secrets"];
	const char* lRootCa = [[myBundle pathForResource:@"rootca"ofType:@"pem"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	connectivity=none;
	signal(SIGPIPE, SIG_IGN);
	//log management	
	
	libmsilbc_init();
#if defined (HAVE_SILK)
    libmssilk_init(); 
#endif	
#ifdef HAVE_AMR
    libmsamr_init(); //load amr plugin if present from the liblinphone sdk
#endif	
#ifdef HAVE_X264
	libmsx264_init(); //load x264 plugin if present from the liblinphone sdk
#endif

#if HAVE_G729
	libmsbcg729_init(); // load g729 plugin
#endif
	/* Initialize linphone core*/
	
    [LinphoneLogger logc:LinphoneLoggerLog format:"Create linphonecore"];
	theLinphoneCore = linphone_core_new (&linphonec_vtable
										 , [confiFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]
										 , [factoryConfig cStringUsingEncoding:[NSString defaultCStringEncoding]]
										 ,self);
    
    linphone_core_set_root_ca(theLinphoneCore, lRootCa);
	// Set audio assets
	const char* lRing = [[myBundle pathForResource:@"oldphone-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(theLinphoneCore, lRing );
	const char* lRingBack = [[myBundle pathForResource:@"ringback"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(theLinphoneCore, lRingBack);
    const char* lPlay = [[myBundle pathForResource:@"toy-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_play_file(theLinphoneCore, lPlay);
	
	linphone_core_set_zrtp_secrets_file(theLinphoneCore, [zrtpSecretsFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    
    [self setupNetworkReachabilityCallback];
	
	// start scheduler
	mIterateTimer = [NSTimer scheduledTimerWithTimeInterval:0.1 
													 target:self 
												   selector:@selector(iterate) 
												   userInfo:nil 
													repeats:YES];
	//init audio session
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];
	BOOL bAudioInputAvailable= [audioSession inputIsAvailable];
    [audioSession setDelegate:self];
	
	NSError* err;
	[audioSession setActive:NO error: &err]; 
	if(!bAudioInputAvailable){
		UIAlertView* error = [[UIAlertView alloc]	initWithTitle:NSLocalizedString(@"No microphone",nil)
														message:NSLocalizedString(@"You need to plug a microphone to your device to use this application.",nil) 
													   delegate:nil 
											  cancelButtonTitle:NSLocalizedString(@"Ok",nil) 
											  otherButtonTitles:nil ,nil];
		[error show];
        [error release];
	}
    
    NSString* path = [myBundle pathForResource:@"nowebcamCIF" ofType:@"jpg"];
    if (path) {
        const char* imagePath = [path cStringUsingEncoding:[NSString defaultCStringEncoding]];
        [LinphoneLogger logc:LinphoneLoggerLog format:"Using '%s' as source image for no webcam", imagePath];
        linphone_core_set_static_picture(theLinphoneCore, imagePath);
    }
    
	/*DETECT cameras*/
	frontCamId= backCamId=nil;
	char** camlist = (char**)linphone_core_get_video_devices(theLinphoneCore);
		for (char* cam = *camlist;*camlist!=NULL;cam=*++camlist) {
			if (strcmp(FRONT_CAM_NAME, cam)==0) {
				frontCamId = cam;
				//great set default cam to front
				linphone_core_set_video_device(theLinphoneCore, cam);
			}
			if (strcmp(BACK_CAM_NAME, cam)==0) {
				backCamId = cam;
			}
			
		}

    NSUInteger cpucount = [[NSProcessInfo processInfo] processorCount];
	ms_set_cpu_count(cpucount);

	if (![LinphoneManager isNotIphone3G]){
		PayloadType *pt=linphone_core_find_payload_type(theLinphoneCore,"SILK",24000);
		if (pt) {
			linphone_core_enable_payload_type(theLinphoneCore,pt,FALSE);
			[LinphoneLogger logc:LinphoneLoggerWarning format:"SILK/24000 and video disabled on old iPhone 3G"];
		}
		linphone_core_enable_video(theLinphoneCore, FALSE, FALSE);
	}
    
    if ([LinphoneManager runningOnIpad])
        ms_set_cpu_count(2);
    else
        ms_set_cpu_count(1);
    
    settingsStore = [[LinphoneCoreSettingsStore alloc] init];
    
    [LinphoneLogger logc:LinphoneLoggerWarning format:"Linphone [%s]  started on [%s]"
               ,linphone_core_get_version()
               ,[[UIDevice currentDevice].model cStringUsingEncoding:[NSString defaultCStringEncoding]]];
    
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground) {
		//go directly to bg mode
		[self enterBackgroundMode];
	}	
}


- (void)destroyLibLinphone {
	[mIterateTimer invalidate]; 
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];
	[audioSession setDelegate:nil];
	if (theLinphoneCore != nil) { //just in case application terminate before linphone core initialization
        [LinphoneLogger logc:LinphoneLoggerLog format:"Destroy linphonecore"];
		linphone_core_destroy(theLinphoneCore);
		theLinphoneCore = nil;
        SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        if (proxyReachability)
            CFRelease(proxyReachability);
        proxyReachability=nil;
        
    }
}

- (BOOL)enterBackgroundMode {
	LinphoneProxyConfig* proxyCfg;
	linphone_core_get_default_proxy(theLinphoneCore, &proxyCfg);	
	linphone_core_stop_dtmf_stream(theLinphoneCore);
	
	if (proxyCfg && [settingsStore boolForKey:@"backgroundmode_preference"]) {
		//For registration register
		linphone_core_refresh_registers(theLinphoneCore);
		
		
		//wait for registration answer
		int i=0;
		while (!linphone_proxy_config_is_registered(proxyCfg) && i++<40 ) {
			linphone_core_iterate(theLinphoneCore);
			usleep(100000);
		}
		//register keepalive
		if ([[UIApplication sharedApplication] setKeepAliveTimeout:600/*(NSTimeInterval)linphone_proxy_config_get_expires(proxyCfg)*/ 
														   handler:^{
															   [LinphoneLogger logc:LinphoneLoggerWarning format:"keepalive handler"];
															   if (theLinphoneCore == nil) {
																   [LinphoneLogger logc:LinphoneLoggerWarning format:"It seems that Linphone BG mode was deactivated, just skipping"];
																   return;
															   }
															   //kick up network cnx, just in case
															   [self kickOffNetworkConnection];
															   [self refreshRegisters];
															   linphone_core_iterate(theLinphoneCore);
														   }
			 ]) {
			
			
			[LinphoneLogger logc:LinphoneLoggerLog format:"keepalive handler succesfully registered"]; 
		} else {
			[LinphoneLogger logc:LinphoneLoggerLog format:"keepalive handler cannot be registered"];
		}
		LCSipTransports transportValue;
		if (linphone_core_get_sip_transports(theLinphoneCore, &transportValue)) {
			[LinphoneLogger logc:LinphoneLoggerError format:"cannot get current transport"];	
		}
		return YES;
	}
	else {
		[LinphoneLogger logc:LinphoneLoggerLog format:"Entering lite bg mode"];
		[self destroyLibLinphone];
        return NO;
	}
}

- (void)becomeActive {
    [self refreshRegisters];
    
	/*IOS specific*/
	linphone_core_start_dtmf_stream(theLinphoneCore);
}

- (void)beginInterruption {
    LinphoneCall* c = linphone_core_get_current_call(theLinphoneCore);
    [LinphoneLogger logc:LinphoneLoggerLog format:"Sound interruption detected!"];
    if (c) {
        linphone_core_pause_call(theLinphoneCore, c);
    }
}

- (void)endInterruption {
    [LinphoneLogger logc:LinphoneLoggerLog format:"Sound interruption ended!"];
}

- (void)refreshRegisters{
	/*first check if network is available*/
	if (proxyReachability){
		SCNetworkReachabilityFlags flags=0;
		if (!SCNetworkReachabilityGetFlags(proxyReachability, &flags)) {
			[LinphoneLogger logc:LinphoneLoggerError format:"Cannot get reachability flags, re-creating reachability context."];
			[self setupNetworkReachabilityCallback];
		}else{
			networkReachabilityCallBack(proxyReachability, flags, 0);
			if (flags==0){
				/*workaround iOS bug: reachability API cease to work after some time.*/
				/*when flags==0, either we have no network, or the reachability object lies. To workaround, create a new one*/
				[self setupNetworkReachabilityCallback];
			}
		}
	}else [LinphoneLogger logc:LinphoneLoggerError format:"No proxy reachability context created !"];
	linphone_core_refresh_registers(theLinphoneCore);//just to make sure REGISTRATION is up to date
}


#pragma mark - Speaker Functions

- (void)enableSpeaker:(BOOL)enable {
    //redirect audio to speaker
    if(enable) {
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
        AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
                                 , sizeof (audioRouteOverride)
                                 , &audioRouteOverride);
    } else {
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
        AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
                                 , sizeof (audioRouteOverride)
                                 , &audioRouteOverride);
    }
}

- (BOOL)isSpeakerEnabled {
    bool enabled = false;
    CFStringRef lNewRoute = CFSTR("Unknown");
    UInt32 lNewRouteSize = sizeof(lNewRoute);
    OSStatus lStatus = AudioSessionGetProperty(kAudioSessionProperty_AudioRoute, &lNewRouteSize, &lNewRoute);
    if (!lStatus && lNewRouteSize > 0) {
        NSString *route = (NSString *) lNewRoute;
        [LinphoneLogger logc:LinphoneLoggerLog format:"Current audio route is [%s]", [route cStringUsingEncoding:[NSString defaultCStringEncoding]]];
        enabled = [route isEqualToString: @"Speaker"] || [route isEqualToString: @"SpeakerAndMicrophone"];
        CFRelease(lNewRoute);
    }
    return enabled;
}


#pragma mark - Call Functions

- (void)call:(NSString *)address displayName:(NSString*)displayName transfer:(BOOL)transfer {
    if (!linphone_core_is_network_reachabled(theLinphoneCore)) {
		UIAlertView* error = [[UIAlertView alloc]	initWithTitle:NSLocalizedString(@"Network Error",nil)
														message:NSLocalizedString(@"There is no network connection available, enable WIFI or WWAN prior to place a call",nil) 
													   delegate:nil 
											  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
											  otherButtonTitles:nil];
		[error show];
        [error release];
		return;
	}
    
    CTCallCenter* ct = [[CTCallCenter alloc] init];
    if ([ct.currentCalls count] > 0) {
        [LinphoneLogger logc:LinphoneLoggerError format:"GSM call in progress, cancelling outgoing SIP call request"];
		UIAlertView* error = [[UIAlertView alloc]	initWithTitle:NSLocalizedString(@"Cannot make call",nil)
														message:NSLocalizedString(@"Please terminate GSM call",nil) 
													   delegate:nil 
											  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
											  otherButtonTitles:nil];
		[error show];
        [error release];
        [ct release];
		return;
    }
    [ct release];
    
	LinphoneProxyConfig* proxyCfg;	
	//get default proxy
	linphone_core_get_default_proxy([LinphoneManager getLc],&proxyCfg);
	LinphoneCallParams* lcallParams = linphone_core_create_default_call_parameters([LinphoneManager getLc]);

	if ([address length] == 0) return; //just return
	if ([address hasPrefix:@"sip:"]) {
        LinphoneAddress* linphoneAddress = linphone_address_new([address cStringUsingEncoding:[NSString defaultCStringEncoding]]);  
        if(displayName!=nil) {
            linphone_address_set_display_name(linphoneAddress,[displayName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        }
        if(transfer) {
            linphone_core_transfer_call([LinphoneManager getLc], linphone_core_get_current_call([LinphoneManager getLc]), [address cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        } else {
            linphone_core_invite_address_with_params([LinphoneManager getLc], linphoneAddress, lcallParams);
        }
        linphone_address_destroy(linphoneAddress);
	} else if (proxyCfg==nil){
		UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Invalid sip address",nil)
														message:NSLocalizedString(@"Either configure a SIP proxy server from settings prior to place a call or use a valid sip address (I.E sip:john@example.net)",nil) 
													   delegate:nil 
											  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
											  otherButtonTitles:nil];
		[error show];
		[error release];
	} else {
		char normalizedUserName[256];
        LinphoneAddress* linphoneAddress = linphone_address_new(linphone_core_get_identity([LinphoneManager getLc]));  
		linphone_proxy_config_normalize_number(proxyCfg,[address cStringUsingEncoding:[NSString defaultCStringEncoding]],normalizedUserName,sizeof(normalizedUserName));
        linphone_address_set_username(linphoneAddress, normalizedUserName);
        if(displayName!=nil) {
            linphone_address_set_display_name(linphoneAddress, [displayName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        }
        if(transfer) {
            linphone_core_transfer_call([LinphoneManager getLc], linphone_core_get_current_call([LinphoneManager getLc]), normalizedUserName);
        } else {
            linphone_core_invite_address_with_params([LinphoneManager getLc], linphoneAddress, lcallParams);
        }
        linphone_address_destroy(linphoneAddress);
	}
	linphone_call_params_destroy(lcallParams);
}


#pragma mark - Property Functions

- (void)setPushNotificationToken:(NSData *)apushNotificationToken {
    if(apushNotificationToken == pushNotificationToken) {
        return;
    }
    if(pushNotificationToken != nil) {
        [pushNotificationToken release];
        pushNotificationToken = nil;
    }
    
    if(apushNotificationToken != nil) {
        pushNotificationToken = [apushNotificationToken retain];
    }
    if([LinphoneManager isLcReady]) {
        [(LinphoneCoreSettingsStore*)settingsStore synchronizeAccount];
    }
}

@end
