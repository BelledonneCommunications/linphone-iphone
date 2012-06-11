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


#import "LinphoneManager.h"
#include "linphonecore_utils.h"
#include "lpconfig.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import "FastAddressBook.h"
#include <sys/sysctl.h>
#include <SystemConfiguration/SystemConfiguration.h>

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
@synthesize callDelegate;
@synthesize registrationDelegate;
@synthesize connectivity;
@synthesize frontCamId;
@synthesize backCamId;
@synthesize isbackgroundModeEnabled;
@synthesize defaultExpires;

-(id) init {
    assert (!theLinphoneManager);
    if ((self= [super init])) {
        mFastAddressBook = [[FastAddressBook alloc] init];
        theLinphoneManager = self;
		self.defaultExpires=600;
    }
    return self;
}
+(LinphoneManager*) instance {
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

-(void) onCall:(LinphoneCall*) call StateChanged: (LinphoneCallState) new_state withMessage: (const char *)  message {
    const char* lUserNameChars=linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    if (new_state == LinphoneCallIncomingReceived) {
       [self updateCallWithAddressBookData:call]; // display name is updated 
    }
    const char* lDisplayNameChars =  linphone_address_get_display_name(linphone_call_get_remote_address(call));        
	NSString* lDisplayName = [lDisplayNameChars?[[NSString alloc] initWithUTF8String:lDisplayNameChars]:@"" autorelease];
    
    bool canHideInCallView = (linphone_core_get_calls([LinphoneManager getLc]) == NULL);
	
    if (!linphone_call_get_user_pointer(call)) {
        LinphoneCallAppData* data = (LinphoneCallAppData*) malloc(sizeof(LinphoneCallAppData));
        data->batteryWarningShown = FALSE;
        linphone_call_set_user_pointer(call, data);
    }
    
	switch (new_state) {					
		case LinphoneCallIncomingReceived: 
			[callDelegate	displayIncomingCall:call 
                           NotificationFromUI:mCurrentViewController
														forUser:lUserName 
												withDisplayName:lDisplayName];
			break;
			
		case LinphoneCallOutgoingInit: 
			[callDelegate		displayCall:call 
                      InProgressFromUI:mCurrentViewController
											   forUser:lUserName 
									   withDisplayName:lDisplayName];
			break;
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
			[callDelegate	displayInCall: call 
                                 FromUI:mCurrentViewController
									  forUser:lUserName 
							  withDisplayName:lDisplayName];
			break;
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);

            /* remote wants to add video */
            if (!linphone_call_params_video_enabled(current) && linphone_call_params_video_enabled(remote) && !linphone_core_get_video_policy(theLinphoneCore)->automatically_accept) {
                linphone_core_defer_call_update(theLinphoneCore, call);
                [callDelegate displayAskToEnableVideoCall:call forUser:lUserName withDisplayName:lDisplayName];
            } else if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [callDelegate displayInCall:call FromUI:mCurrentViewController forUser:lUserName withDisplayName:lDisplayName];
            }
            break;
        }
        case LinphoneCallUpdated:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            if (linphone_call_params_video_enabled(current)) {
                [callDelegate displayVideoCall:call FromUI:mCurrentViewController forUser:lUserName withDisplayName:lDisplayName];
            } else {
                [callDelegate displayInCall:call FromUI:mCurrentViewController forUser:lUserName withDisplayName:lDisplayName];
            }
            break;
            
        }
		case LinphoneCallError: { 
			/*
			 NSString* lTitle= state->message!=nil?[NSString stringWithCString:state->message length:strlen(state->message)]: @"Error";
			 NSString* lMessage=lTitle;
			 */
			NSString* lMessage;
			NSString* lTitle;
			LinphoneProxyConfig* proxyCfg;	
			//get default proxy
			linphone_core_get_default_proxy([LinphoneManager getLc],&proxyCfg);
			if (proxyCfg == nil) {
				lMessage=NSLocalizedString(@"Please make sure your device is connected to the internet and double check your SIP account configuration in the settings.",nil);
			} else {
				lMessage=[NSString stringWithFormat : NSLocalizedString(@"Cannot call %@",nil),lUserName];
			}
			
            if (linphone_call_get_reason(call) == LinphoneReasonNotFound) {
                lMessage=[NSString stringWithFormat : NSLocalizedString(@"'%@' not registered to Service",nil), lUserName];
            } else {
                if (message!=nil){
                    lMessage=[NSString stringWithFormat : NSLocalizedString(@"%@\nReason was: %s",nil),lMessage, message];
                }
            }
			lTitle=NSLocalizedString(@"Call failed",nil);
			
			UIAlertView* error = [[UIAlertView alloc] initWithTitle:lTitle
															message:lMessage 
														   delegate:nil 
												  cancelButtonTitle:NSLocalizedString(@"Dismiss",nil) 
												  otherButtonTitles:nil];
			[error show];
            [error release];
            if (canHideInCallView) {
                [callDelegate	displayDialerFromUI:mCurrentViewController
									  forUser:@"" 
							  withDisplayName:@""];
            } else {
				[callDelegate	displayInCall:call 
									 FromUI:mCurrentViewController
									forUser:lUserName 
							withDisplayName:lDisplayName];	
			}
			break;
		}
		case LinphoneCallEnd:
            if (canHideInCallView) {
                [callDelegate	displayDialerFromUI:mCurrentViewController
									  forUser:@"" 
							  withDisplayName:@""];
            } else {
				[callDelegate	displayInCall:call 
									 FromUI:mCurrentViewController
									forUser:lUserName 
							withDisplayName:lDisplayName];	
			}
			break;
		case LinphoneCallStreamsRunning:
			//check video
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				[callDelegate	displayVideoCall:call FromUI:mCurrentViewController
                                       forUser:lUserName 
                               withDisplayName:lDisplayName];
			} else {
                [callDelegate displayInCall:call FromUI:mCurrentViewController forUser:lUserName withDisplayName:lDisplayName];
            }
			break;
        case LinphoneCallReleased:
            free (linphone_call_get_user_pointer(call));
            break;
        default:
            break;
	}
	
}

-(void) displayDialer {
    [callDelegate	displayDialerFromUI:mCurrentViewController
                              forUser:@"" 
                      withDisplayName:@""];
}

+(LinphoneCore*) getLc {
	if (theLinphoneCore==nil) {
		@throw([NSException exceptionWithName:@"LinphoneCoreException" reason:@"Linphone core not initialized yet" userInfo:nil]);
	}
	return theLinphoneCore;
}

-(void) addLog:(NSString*) log {
	[mLogView addLog:log];
}
-(void)displayStatus:(NSString*) message {
	[callDelegate displayStatus:message];
}
//generic log handler for debug version
static void linphone_iphone_log_handler(int lev, const char *fmt, va_list args){
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
    
	LinphoneAddress* lAddress = linphone_address_new(linphone_proxy_config_get_identity(cfg));
	NSString* lUserName = linphone_address_get_username(lAddress)? [[NSString alloc] initWithUTF8String:linphone_address_get_username(lAddress) ]:@"";
	NSString* lDisplayName = linphone_address_get_display_name(lAddress)? [[NSString alloc] initWithUTF8String:linphone_address_get_display_name(lAddress) ]:@"";
	NSString* lDomain = [[NSString alloc] initWithUTF8String:linphone_address_get_domain(lAddress)];
	linphone_address_destroy(lAddress);
	
	if (state == LinphoneRegistrationOk) {
		[[(LinphoneManager*)linphone_core_get_user_data(lc) registrationDelegate] displayRegisteredFromUI:nil
											  forUser:lUserName
									  withDisplayName:lDisplayName
											 onDomain:lDomain ];
	} else if (state == LinphoneRegistrationProgress) {
		[registrationDelegate displayRegisteringFromUI:mCurrentViewController
											  forUser:lUserName
									  withDisplayName:lDisplayName
											 onDomain:lDomain ];
		
	} else if (state == LinphoneRegistrationCleared || state == LinphoneRegistrationNone) {
		[registrationDelegate displayNotRegisteredFromUI:mCurrentViewController];
	} else 	if (state == LinphoneRegistrationFailed ) {
		NSString* lErrorMessage=nil;
		if (linphone_proxy_config_get_error(cfg) == LinphoneReasonBadCredentials) {
			lErrorMessage = NSLocalizedString(@"Bad credentials, check your account settings",nil);
		} else if (linphone_proxy_config_get_error(cfg) == LinphoneReasonNoResponse) {
			lErrorMessage = NSLocalizedString(@"SIP server unreachable",nil);
		} 
		[registrationDelegate displayRegistrationFailedFromUI:mCurrentViewController
											   forUser:lUserName
									   withDisplayName:lDisplayName
											  onDomain:lDomain
												forReason:lErrorMessage];
		
		if (lErrorMessage != nil 
			&& linphone_proxy_config_get_error(cfg) != LinphoneReasonNoResponse) { //do not report network connection issue on registration
			//default behavior if no registration delegates
			UIApplicationState s = [UIApplication sharedApplication].applicationState;
            
            // do not stack error message when going to backgroud
            if (s != UIApplicationStateBackground) {
                UIAlertView* error = [[UIAlertView alloc]	initWithTitle:NSLocalizedString(@"Registration failure",nil)
															message:lErrorMessage
														   delegate:nil 
												  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
												  otherButtonTitles:nil ,nil];
                [error show];
                [error release];
            }
		}
		
	}
    
    [lUserName release];
    [lDisplayName release];
    [lDomain release];
	
}
static void linphone_iphone_registration_state(LinphoneCore *lc, LinphoneProxyConfig* cfg, LinphoneRegistrationState state,const char* message) {
	[(LinphoneManager*)linphone_core_get_user_data(lc) onRegister:lc cfg:cfg state:state message:message];
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
	.text_received=NULL,
	.dtmf_received=NULL,
    .transfer_state_changed=linphone_iphone_transfer_state_changed
};


-(void) kickOffNetworkConnection {
	/*start a new thread to avoid blocking the main ui in case of peer host failure*/
	[NSThread detachNewThreadSelector:@selector(runNetworkConnection) toTarget:self withObject:nil];
}
-(void) runNetworkConnection {
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
	
}
-(void) destroyLibLinphone {
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
-(BOOL) enterBackgroundMode {
	LinphoneProxyConfig* proxyCfg;
	linphone_core_get_default_proxy(theLinphoneCore, &proxyCfg);	
	linphone_core_stop_dtmf_stream(theLinphoneCore);
	
	if (isbackgroundModeEnabled && proxyCfg) {
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
-(void) iterate {
	linphone_core_iterate(theLinphoneCore);
}

-(void) setupNetworkReachabilityCallback: (const char*) nodeName withContext:(SCNetworkReachabilityContext*) ctx {
    if (proxyReachability) {
        ms_message("Cancel old network reachability check");
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
-(void)startLibLinphone  {
	
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
	const char*  lRing = [[myBundle pathForResource:@"oldphone-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(theLinphoneCore, lRing );
	const char*  lRingBack = [[myBundle pathForResource:@"ringback"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(theLinphoneCore, lRingBack);

	
	linphone_core_set_zrtp_secrets_file(theLinphoneCore, [zrtpSecretsFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    
    [self setupNetworkReachabilityCallback: "linphone.org" withContext:nil];
	
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

	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground) {
		//go directly to bg mode
		[self enterBackgroundMode];
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
    
    ms_warning("Linphone [%s]  started on [%s], running with [%u] processor(s)"
               ,linphone_core_get_version()
               ,[[UIDevice currentDevice].model cStringUsingEncoding:[NSString defaultCStringEncoding]],
			   cpucount);
	
}

-(void) refreshRegisters{
	/*first check if network is available*/
	if (proxyReachability){
		SCNetworkReachabilityFlags flags=0;
		if (!SCNetworkReachabilityGetFlags(proxyReachability, &flags)) {
			ms_error("Cannot get reachability flags");
		}else
			networkReachabilityCallBack(proxyReachability, flags, 0);	
	}else ms_error("No proxy reachability context created !");
	linphone_core_refresh_registers(theLinphoneCore);//just to make sure REGISTRATION is up to date
}

-(void) becomeActive {
    if (theLinphoneCore == nil) {
		//back from standby and background mode is disabled
		[self	startLibLinphone];
	} else {
		[self refreshRegisters];
	}
	/*IOS specific*/
	linphone_core_start_dtmf_stream(theLinphoneCore);
}

-(void) registerLogView:(id<LogView>) view {
	mLogView = view;
}

-(void) beginInterruption {
    LinphoneCall* c = linphone_core_get_current_call(theLinphoneCore);
    ms_message("Sound interruption detected!");
    if (c) {
        linphone_core_pause_call(theLinphoneCore, c);
    }
}

-(void) endInterruption {
    ms_message("Sound interruption ended!");
    //let the user resume the call manually.
}
+(BOOL) runningOnIpad {
#ifdef UI_USER_INTERFACE_IDIOM
    return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
#endif
    return NO;
}

+(void) set:(UIView*)view hidden: (BOOL) hidden withName:(const char*)name andReason:(const char*) reason{
    if (view.hidden != hidden) {
        ms_message("UI - '%s' is now '%s' ('%s')", name, hidden ? "HIDDEN" : "SHOWN", reason);
        [view setHidden:hidden];
    }
}

+(void) logUIElementPressed:(const char*) name {
    ms_message("UI - '%s' pressed", name);
}

-(void) settingsViewControllerDidEnd:(IASKAppSettingsViewController *)sender {
    NSLog(@"settingsViewControllerDidEnd");
}

-(NSDictionary*) filterPreferenceSpecifier:(NSDictionary *)specifier {
    if (!theLinphoneCore) {
        // LinphoneCore not ready: do not filter
        return specifier;
    }
    NSString* identifier = [specifier objectForKey:@"Identifier"];
    if (identifier == nil) {
        identifier = [specifier objectForKey:@"Key"];
    }
    if (!identifier) {
        // child pane maybe
        NSString* title = [specifier objectForKey:@"Title"];
        if ([title isEqualToString:@"Video"]) {
            if (!linphone_core_video_supported(theLinphoneCore))
                return nil;
        }
        return specifier;
    }
    // NSLog(@"Specifier received: %@", identifier);
    if ([identifier hasPrefix:@"silk"]) {
		if (linphone_core_find_payload_type(theLinphoneCore,"SILK",8000)==NULL){
			return nil;
		}
		if ([identifier isEqualToString:@"silk_24k_preference"]) {
			if (![self isNotIphone3G])
				return nil;
		}
		
    } else if ([identifier isEqualToString:@"backgroundmode_preference"]) {
        UIDevice* device = [UIDevice currentDevice];
        if ([device respondsToSelector:@selector(isMultitaskingSupported)]) {
            if ([device isMultitaskingSupported]) {
                return specifier;
            }
        }
        // hide setting if bg mode not supported
        return nil;
    }
    return specifier;
}



@end
