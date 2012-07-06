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

#import "LinphoneManager.h"
#import "FastAddressBook.h"
#import "LinphoneCoreSettingsStore.h"
#import "ChatModel.h"

#include "linphonecore_utils.h"
#include "lpconfig.h"
#include "private.h"

static LinphoneCore* theLinphoneCore=nil;
static LinphoneManager* theLinphoneManager=nil;

extern void libmsilbc_init();
#ifdef HAVE_AMR
extern void libmsamr_init();
#endif

#ifdef HAVE_X264
extern void libmsx264_init();
#endif
#define FRONT_CAM_NAME "AV Capture: Front Camera"
#define BACK_CAM_NAME "AV Capture: Back Camera"

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

+ (BOOL)codecIsSupported:(NSString *) prefName{
	int i;
	for(i=0;codec_pref_table[i].name!=NULL;++i){
		if ([prefName compare:codec_pref_table[i].prefname]==0){
			return linphone_core_find_payload_type([LinphoneManager getLc],codec_pref_table[i].name, codec_pref_table[i].rate)!=NULL;
		}
	}
	return TRUE;
}

- (id)init {
    assert (!theLinphoneManager);
    if ((self = [super init])) {
        mFastAddressBook = [[FastAddressBook alloc] init];
        database = NULL;
        theLinphoneManager = self;
		self.defaultExpires = 600;
        [self openDatabase];
    }
    return self;
}

- (void)dealloc {
    [mFastAddressBook release];
    [self closeDatabase];
    
    [super dealloc];
}

- (void)openDatabase {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsPath = [paths objectAtIndex:0];
    NSString *databaseDocumentPath = [documentsPath stringByAppendingPathComponent:@"database.txt"];

    // Copy default database
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error = nil;
    //[fileManager removeItemAtPath:databaseDocumentPath error:&error]; //TODO REMOVE
    if ([fileManager fileExistsAtPath:databaseDocumentPath] == NO) {
        NSLog(@"Create sqlite 3 database");
        NSString *resourceDocumentPath = [[NSBundle mainBundle] pathForResource:@"database" ofType:@"sqlite"];
        [fileManager copyItemAtPath:resourceDocumentPath toPath:databaseDocumentPath error:&error];
        if(error != nil) {
            NSLog(@"Can't copy database: %@", [error localizedDescription]);
            return;
        }
    }

    if(sqlite3_open([databaseDocumentPath UTF8String], &database) != SQLITE_OK) {
        NSLog(@"Can't open \"%@\" sqlite3 database.", databaseDocumentPath);
    }
}

- (void)closeDatabase {
    if(database != NULL) {
        if(sqlite3_close(database) != SQLITE_OK) {
            NSLog(@"Can't close sqlite3 database.");
        }
    }
}

+ (LinphoneManager*)instance {
	return theLinphoneManager;
}

-(NSString*) getDisplayNameFromAddressBook:(NSString*) number andUpdateCallLog:(LinphoneCallLog*)log {
    //1 normalize
    NSString* lNormalizedNumber = [FastAddressBook normalizePhoneNumber:number];
    Contact* lContact = [mFastAddressBook getMatchingRecord:lNormalizedNumber];
    if (lContact) {
        CFStringRef lDisplayName = ABRecordCopyCompositeName(lContact.record);
        
        if (log) {
            //add phone type
            char ltmpString[256];
            CFStringRef lFormatedString = CFStringCreateWithFormat(NULL,NULL,CFSTR("phone_type:%@;"),lContact.numberType);
            CFStringGetCString(lFormatedString, ltmpString,sizeof(ltmpString), kCFStringEncodingUTF8);
            linphone_call_log_set_ref_key(log, ltmpString);
            CFRelease(lFormatedString);
        }
        return [(NSString*)lDisplayName autorelease];   
    }
    //[number release];
 
    return nil;
}

-(UIImage*) getImageFromAddressBook:(NSString *)number {
    NSString* lNormalizedNumber = [FastAddressBook normalizePhoneNumber:number];
    Contact* lContact = [mFastAddressBook getMatchingRecord:lNormalizedNumber];
    if (lContact) {
        ABRecordRef person = ABAddressBookGetPersonWithRecordID(mFastAddressBook.addressBook, ABRecordGetRecordID(lContact.record));            
        if (ABPersonHasImageData(person)) {
            NSData* d;
            // ios 4.1+
            if ( &ABPersonCopyImageDataWithFormat != nil) {
                d = (NSData*)ABPersonCopyImageDataWithFormat(person, kABPersonImageFormatThumbnail);
            } else {
                d = (NSData*)ABPersonCopyImageData(person);
            }
            return [UIImage imageWithData:[d autorelease]];
        }
    }
    /* return default image */
    return [UIImage imageWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"contact_vide" ofType:@"png"]];
    //return nil;
}

-(void) updateCallWithAddressBookData:(LinphoneCall*) call {
    //1 copy adress book
    LinphoneCallLog* lLog = linphone_call_get_call_log(call);
    LinphoneAddress* lAddress;
    if (lLog->dir == LinphoneCallIncoming) {
        lAddress=lLog->from;
    } else {
        lAddress=lLog->to;
    }
    const char* lUserName = linphone_address_get_username(lAddress); 
    if (!lUserName) {
        //just return
        return;
    }
    
    NSString* lE164Number = [[NSString alloc] initWithCString:lUserName encoding:[NSString defaultCStringEncoding]];
    NSString* lDisplayName = [self getDisplayNameFromAddressBook:lE164Number andUpdateCallLog:lLog];
    
    if(lDisplayName) {        
        linphone_address_set_display_name(lAddress, [lDisplayName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    } else {
        ms_message("No contact entry found for  [%s] in address book",lUserName);
    }
    
    [lE164Number release];
    return;
}

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
        linphone_call_set_user_pointer(call, data);
    }
    
    if (state == LinphoneCallIncomingReceived) {
        [self updateCallWithAddressBookData:call]; // display name is updated 
    }
    
    if ((state == LinphoneCallEnd || state == LinphoneCallError)) {
        if(linphone_core_get_calls_nb([LinphoneManager getLc]) == 0)
            [self enableSpeaker:FALSE];
    }
    
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                          [NSValue valueWithPointer:call], @"call",
                          [NSNumber numberWithInt:state], @"state", 
                          [NSString stringWithUTF8String:message], @"message", nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneCallUpdate" object:self userInfo:dict];
}

+ (LinphoneCore*)getLc {
	if (theLinphoneCore==nil) {
		@throw([NSException exceptionWithName:@"LinphoneCoreException" reason:@"Linphone core not initialized yet" userInfo:nil]);
	}
	return theLinphoneCore;
}

+ (BOOL)isLcReady {
    return theLinphoneCore != nil;
}

- (void)addLog:(NSString*) log {
	[mLogView addLog:log];
}

- (void)displayStatus:(NSString*) message {
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                          message, @"message", 
                          nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneDisplayStatus" object:self userInfo:dict];
}

//generic log handler for debug version
void linphone_iphone_log_handler(int lev, const char *fmt, va_list args){
	NSString* format = [[NSString alloc] initWithCString:fmt encoding:[NSString defaultCStringEncoding]];
	NSLogv(format,args);
	NSString* formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	[[LinphoneManager instance] addLog:formatedString];
	[format release];
	[formatedString release];
}

//Error/warning log handler 
static void linphone_iphone_log(struct _LinphoneCore * lc, const char * message) {
	NSString* log = [NSString stringWithCString:message encoding:[NSString defaultCStringEncoding]]; 
	NSLog(log,NULL);
	[[LinphoneManager instance]addLog:log];
}
//status 
static void linphone_iphone_display_status(struct _LinphoneCore * lc, const char * message) {
    NSString* status = [[NSString alloc] initWithCString:message encoding:[NSString defaultCStringEncoding]];
	[(LinphoneManager*)linphone_core_get_user_data(lc)  displayStatus:status];
    [status release];
}

static void linphone_iphone_call_state(LinphoneCore *lc, LinphoneCall* call, LinphoneCallState state,const char* message) {
	/*LinphoneCallIdle,
	 LinphoneCallIncomingReceived,
	 LinphoneCallOutgoingInit,
	 LinphoneCallOutgoingProgress,
	 LinphoneCallOutgoingRinging,
	 LinphoneCallOutgoingEarlyMedia,
	 LinphoneCallConnected,
	 LinphoneCallStreamsRunning,
	 LinphoneCallPausing,
	 LinphoneCallPaused,
	 LinphoneCallResuming,
	 LinphoneCallRefered,
	 LinphoneCallError,
	 LinphoneCallEnd,
	 LinphoneCallPausedByRemote
	 */
	[(LinphoneManager*)linphone_core_get_user_data(lc) onCall:call StateChanged: state withMessage:  message];
}

static void linphone_iphone_transfer_state_changed(LinphoneCore* lc, LinphoneCall* call, LinphoneCallState state) {
    /*
     LinhoneCallOutgoingProgress -> SalReferTrying
     LinphoneCallConnected -> SalReferSuccess
     LinphoneCallError -> SalReferFailed | *
     */
}

-(void) onRegister:(LinphoneCore *)lc cfg:(LinphoneProxyConfig*) cfg state:(LinphoneRegistrationState) state message:(const char*) message {
    ms_warning("NEW REGISTRATION STATE: '%s' (message: '%s')", linphone_registration_state_to_string(state), message);
    
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

- (void)onTextReceived:(LinphoneCore *)lc room:(LinphoneChatRoom *)room from:(const LinphoneAddress *)from message:(const char *)message {
    
    // Save message in database
    ChatModel *chat = [[ChatModel alloc] init];
    [chat setRemoteContact:[NSString stringWithUTF8String:linphone_address_get_username(from)]];
    [chat setMessage:[NSString stringWithUTF8String:message]];
    [chat setDirection:[NSNumber numberWithInt:1]];
    [chat create];
    
    // Post event
    NSDictionary* dict = [[[NSDictionary alloc] initWithObjectsAndKeys: 
                           [NSValue valueWithPointer:room], @"room", 
                           [NSValue valueWithPointer:from], @"from",
                           [NSString stringWithUTF8String:message], @"message", 
                           nil] autorelease];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneTextReceived" object:self userInfo:dict]; 
}

static void linphone_iphone_text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message) {
    [(LinphoneManager*)linphone_core_get_user_data(lc) onTextReceived:lc room:room from:from message:message];
}

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
	ms_message("Network connection flags:");
	if (flags==0) ms_message("no flags.");
	if (flags & kSCNetworkReachabilityFlagsTransientConnection)
		ms_message("kSCNetworkReachabilityFlagsTransientConnection");
	if (flags & kSCNetworkReachabilityFlagsReachable)
		ms_message("kSCNetworkReachabilityFlagsReachable");
	if (flags & kSCNetworkReachabilityFlagsConnectionRequired)
		ms_message("kSCNetworkReachabilityFlagsConnectionRequired");
	if (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic)
		ms_message("kSCNetworkReachabilityFlagsConnectionOnTraffic");
	if (flags & kSCNetworkReachabilityFlagsConnectionOnDemand)
		ms_message("kSCNetworkReachabilityFlagsConnectionOnDemand");
	if (flags & kSCNetworkReachabilityFlagsIsLocalAddress)
		ms_message("kSCNetworkReachabilityFlagsIsLocalAddress");
	if (flags & kSCNetworkReachabilityFlagsIsDirect)
		ms_message("kSCNetworkReachabilityFlagsIsDirect");
	if (flags & kSCNetworkReachabilityFlagsIsWWAN)
		ms_message("kSCNetworkReachabilityFlagsIsWWAN");
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
				ms_message("Network connectivity changed to type [%s]",(newConnectivity==wifi?"wifi":"wwan"));
			}
			lLinphoneMgr.connectivity=newConnectivity;
		}
		if (ctx && ctx->networkStateChanged) {
            (*ctx->networkStateChanged)(lLinphoneMgr.connectivity);
        }
	}
}

- (BOOL)isNotIphone3G
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

// no proxy configured alert 
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 1) {
		[[[LinphoneManager instance] settingsStore] setBool:true forKey:@"check_config_disable_preference"];
	}
}

- (void)destroyLibLinphone {
	[mIterateTimer invalidate]; 
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];
	[audioSession setDelegate:nil];
	if (theLinphoneCore != nil) { //just in case application terminate before linphone core initialization
        NSLog(@"Destroy linphonecore");
		linphone_core_destroy(theLinphoneCore);
		theLinphoneCore = nil;
        SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        if (proxyReachability)
            CFRelease(proxyReachability);
        proxyReachability=nil;
        
    }
    
}

//**********************BG mode management*************************///////////
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
															   ms_warning("keepalive handler");
															   if (theLinphoneCore == nil) {
																   ms_warning("It seems that Linphone BG mode was deactivated, just skipping");
																   return;
															   }
															   //kick up network cnx, just in case
															   [self kickOffNetworkConnection];
															   [self refreshRegisters];
															   linphone_core_iterate(theLinphoneCore);
														   }
			 ]) {
			
			
			ms_message("keepalive handler succesfully registered"); 
		} else {
			ms_warning("keepalive handler cannot be registered");
		}
		LCSipTransports transportValue;
		if (linphone_core_get_sip_transports(theLinphoneCore, &transportValue)) {
			ms_error("cannot get current transport");	
		}
		return YES;
	}
	else {
		ms_message("Entering lite bg mode");
		[self destroyLibLinphone];
        return NO;
	}
}

//scheduling loop
- (void)iterate {
	linphone_core_iterate(theLinphoneCore);
}

- (void)setupNetworkReachabilityCallback {
	SCNetworkReachabilityContext *ctx=NULL;
	const char *nodeName="linphone.org";
	
    if (proxyReachability) {
        ms_message("Cancelling old network reachability");
        SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(proxyReachability);
        proxyReachability = nil;
    }
    
    proxyReachability = SCNetworkReachabilityCreateWithName(nil, nodeName);
    
	//initial state is network off should be done as soon as possible
	SCNetworkReachabilityFlags flags;
	if (!SCNetworkReachabilityGetFlags(proxyReachability, &flags)) {
		ms_error("Cannot get reachability flags: %s", SCErrorString(SCError()));
		return;
	}
	networkReachabilityCallBack(proxyReachability, flags, ctx ? ctx->info : 0);

	if (!SCNetworkReachabilitySetCallback(proxyReachability, (SCNetworkReachabilityCallBack)networkReachabilityCallBack, ctx)){
		ms_error("Cannot register reachability cb: %s", SCErrorString(SCError()));
		return;
	}
	if(!SCNetworkReachabilityScheduleWithRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode)){
		ms_error("Cannot register schedule reachability cb: %s", SCErrorString(SCError()));
		return;
	}
}


/*************
 *lib linphone init method
 */
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
	
    NSLog(@"Create linphonecore");
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
        ms_message("Using '%s' as source image for no webcam", imagePath);
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

	if (![self isNotIphone3G]){
		PayloadType *pt=linphone_core_find_payload_type(theLinphoneCore,"SILK",24000);
		if (pt) {
			linphone_core_enable_payload_type(theLinphoneCore,pt,FALSE);
			ms_warning("SILK/24000 and video disabled on old iPhone 3G");
		}
		linphone_core_enable_video(theLinphoneCore, FALSE, FALSE);
	}
    
    if ([LinphoneManager runningOnIpad])
        ms_set_cpu_count(2);
    else
        ms_set_cpu_count(1);
    
    settingsStore = [[LinphoneCoreSettingsStore alloc] init];
    
    ms_warning("Linphone [%s]  started on [%s]"
               ,linphone_core_get_version()
               ,[[UIDevice currentDevice].model cStringUsingEncoding:[NSString defaultCStringEncoding]] );
    
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground) {
		//go directly to bg mode
		[self enterBackgroundMode];
	}	
}

- (void)refreshRegisters{
	/*first check if network is available*/
	if (proxyReachability){
		SCNetworkReachabilityFlags flags=0;
		if (!SCNetworkReachabilityGetFlags(proxyReachability, &flags)) {
			ms_error("Cannot get reachability flags, re-creating reachability context.");
			[self setupNetworkReachabilityCallback];
		}else{
			networkReachabilityCallBack(proxyReachability, flags, 0);
			if (flags==0){
				/*workaround iOS bug: reachability API cease to work after some time.*/
				/*when flags==0, either we have no network, or the reachability object lies. To workaround, create a new one*/
				[self setupNetworkReachabilityCallback];
			}
		}
	}else ms_error("No proxy reachability context created !");
	linphone_core_refresh_registers(theLinphoneCore);//just to make sure REGISTRATION is up to date
}

- (void)becomeActive {
    if (theLinphoneCore == nil) {
		//back from standby and background mode is disabled
		[self startLibLinphone];
	} else {
        [self refreshRegisters];
    }
	/*IOS specific*/
	linphone_core_start_dtmf_stream(theLinphoneCore);
    
}

- (void)registerLogView:(id<LogView>) view {
	mLogView = view;
}

- (void)beginInterruption {
    LinphoneCall* c = linphone_core_get_current_call(theLinphoneCore);
    ms_message("Sound interruption detected!");
    if (c) {
        linphone_core_pause_call(theLinphoneCore, c);
    }
}

- (void)endInterruption {
    ms_message("Sound interruption ended!");
    //let the user resume the call manually.
}

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
    CFStringRef lNewRoute = CFSTR("Unknown");
    UInt32 lNewRouteSize = sizeof(lNewRoute);
    OSStatus lStatus = AudioSessionGetProperty(kAudioSessionProperty_AudioRoute, &lNewRouteSize, &lNewRoute);
    if (!lStatus && lNewRouteSize > 0) {
        NSString *route = (NSString *) lNewRoute;
        ms_message("Current audio route is [%s]", [route cStringUsingEncoding:[NSString defaultCStringEncoding]]);
        return [route isEqualToString: @"Speaker"] || [route isEqualToString: @"SpeakerAndMicrophone"];
    } else {
        return false;
    }
}

+ (BOOL)runningOnIpad {
#ifdef UI_USER_INTERFACE_IDIOM
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
#endif
    return NO;
}

@end
