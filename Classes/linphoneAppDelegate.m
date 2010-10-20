/* linphoneAppDelegate.m
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
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

#import "PhoneViewController.h"
#import "linphoneAppDelegate.h"
#import "ContactPickerDelegate.h"
#import "AddressBook/ABPerson.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import "ConsoleViewController.h"
#import "MoreViewController.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


extern void ms_au_register_card();

//generic log handler for debug version
void linphone_iphone_log_handler(int lev, const char *fmt, va_list args){
	NSString* format = [[NSString alloc] initWithCString:fmt encoding:[NSString defaultCStringEncoding]];
	NSLogv(format,args);
	NSString* formatedString = [[NSString alloc] initWithFormat:format arguments:args];
	[ConsoleViewController addLog:formatedString];
	[format release];
	[formatedString release];
}

//Error/warning log handler 
void linphone_iphone_log(struct _LinphoneCore * lc, const char * message) {
	NSString* log = [NSString stringWithCString:message encoding:[NSString defaultCStringEncoding]]; 
	NSLog(log);
	[ConsoleViewController addLog:log];
}

//status 
void linphone_iphone_display_status(struct _LinphoneCore * lc, const char * message) {
	PhoneViewController* lPhone = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myPhoneViewController;
	[lPhone displayStatus:[NSString stringWithCString:message encoding:[NSString defaultCStringEncoding]]];
}

void linphone_iphone_call_state(LinphoneCore *lc, LinphoneCall* call, LinphoneCallState state,const char* message) {
	linphoneAppDelegate* lAppDelegate = (linphoneAppDelegate*) linphone_core_get_user_data(lc); 
	if (state == LinphoneCallIncomingReceived) {
		[lAppDelegate newIncomingCall:[[NSString alloc] initWithCString:linphone_address_get_username(linphone_call_get_remote_address(call))]]; 
	} else if (lAppDelegate.backgroundSupported && [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground && (LinphoneCallEnd|LinphoneCallError)) {
		// cancel local notif if needed
		[[UIApplication sharedApplication] cancelAllLocalNotifications];
	}
	PhoneViewController* lPhone = lAppDelegate.myPhoneViewController;
	[lPhone onCall:call StateChanged:state withMessage:message];
}

void linphone_iphone_registration_state(LinphoneCore *lc, LinphoneProxyConfig* cfg, LinphoneRegistrationState state,const char* message) {
	linphoneAppDelegate* lAppDelegate = (linphoneAppDelegate*) linphone_core_get_user_data(lc); 
	if (state == LinphoneRegistrationFailed ) {
		
		NSString* lErrorMessage;
		if (linphone_proxy_config_get_error(cfg) == LinphoneErrorBadCredentials) {
			lErrorMessage = @"Bad credentials, check your account settings";
		} else if (linphone_proxy_config_get_error(cfg) == LinphoneErrorNoResponse) {
			lErrorMessage = @"SIP server unreachable";
		} 
		if (lErrorMessage != nil) {
			
			
			UIAlertView* error = [[UIAlertView alloc]	initWithTitle:@"Registration failure"
															message:lErrorMessage
														   delegate:lAppDelegate 
												  cancelButtonTitle:@"Continue" 
												  otherButtonTitles:nil ,nil];
			[error show];
		}
		
	}
	
}


LinphoneCoreVTable linphonec_vtable = {
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
.dtmf_received=NULL
};




@implementation linphoneAppDelegate

@synthesize window;
@synthesize myTabBarController;
@synthesize myPeoplePickerController;
@synthesize myPhoneViewController;
@synthesize backgroundSupported;

- (void)applicationDidEnterBackground:(UIApplication *)application {
	
//#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
	
	struct addrinfo hints;
	struct addrinfo *res=NULL;
	int err;
	
	LinphoneProxyConfig* proxyCfg;
	LinphoneAddress *addr;
	int sipsock = linphone_core_get_sip_socket(myLinphoneCore);	
	linphone_core_get_default_proxy(myLinphoneCore, &proxyCfg);	
	
	if (backgroundSupported && proxyCfg) {
		
		
		if (isbackgroundModeEnabled) {
			//register
			linphone_core_set_network_reachable(myLinphoneCore,false);
			linphone_core_iterate(myLinphoneCore);
			linphone_core_set_network_reachable(myLinphoneCore,true);
			
			int i=0;
			while (!linphone_proxy_config_is_registered(proxyCfg) && i++<40 ) {
				linphone_core_iterate(myLinphoneCore);
				usleep(100000);
			}
			if ([[UIApplication sharedApplication] setKeepAliveTimeout:600/*(NSTimeInterval)linphone_proxy_config_get_expires(proxyCfg)*/ 
															   handler:^{
																   ms_warning("keepalive handler");
																   //kick up network cnx, just in case
																   linphone_core_set_network_reachable(myLinphoneCore,false);
																   linphone_core_iterate(myLinphoneCore);
																   [self kickOffNetworkConnection];
																   linphone_core_set_network_reachable(myLinphoneCore,true);
																   linphone_core_iterate(myLinphoneCore);
															   }
				 ]) {
				
				
				ms_warning("keepalive handler succesfully registered"); 
			} else {
				ms_warning("keepalive handler cannot be registered");
			}
			LCSipTransports transportValue;
			if (linphone_core_get_sip_transports(myLinphoneCore, &transportValue)) {
				ms_error("cannot get current transport");	
			}
			
			if (mReadStream == nil && transportValue.udp_port>0) { //only for udp
				const char *port;
				addr=linphone_address_new(linphone_proxy_config_get_addr(proxyCfg));
				memset(&hints,0,sizeof(hints));
				hints.ai_family=linphone_core_ipv6_enabled(myLinphoneCore) ? AF_INET6 : AF_INET;
				port=linphone_address_get_port(addr);
				if (port==NULL) port="5060";
				err=getaddrinfo(linphone_address_get_domain(addr),port,&hints,&res);
				if (err!=0){
					ms_error("getaddrinfo() failed for %s: %s",linphone_address_get_domain(addr),gai_strerror(err));
					linphone_address_destroy(addr);
					return;
				}
				err=connect(sipsock,res->ai_addr,res->ai_addrlen);
				if (err==-1){
					ms_error("Connect failed: %s",strerror(errno));
				}
				freeaddrinfo(res);
				
				CFStreamCreatePairWithSocket(NULL, (CFSocketNativeHandle)sipsock, &mReadStream,nil);
				
				if (!CFReadStreamSetProperty(mReadStream, kCFStreamNetworkServiceType, kCFStreamNetworkServiceTypeVoIP)) {
					ms_error("cannot set service type to voip for read stream");
				}
				
				
				if (!CFReadStreamOpen(mReadStream)) {
					ms_error("cannot open read stream");
				}		
			}
		}
		else {
			//only unregister
			//register
			linphone_proxy_config_edit(proxyCfg); //force unregister
			linphone_core_iterate(myLinphoneCore);
			ms_warning("Entering lite bg mode");
		}
	}
		
//#endif
		
	}


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions{    
	
	//as defined in PhoneMainView.xib		
#define DIALER_TAB_INDEX 1
#define CONTACTS_TAB_INDEX 2
#define HISTORY_TAB_INDEX 0
#define MORE_TAB_INDEX 3
	
	UIDevice* device = [UIDevice currentDevice];
	backgroundSupported = false;
	if ([device respondsToSelector:@selector(isMultitaskingSupported)])
		backgroundSupported = [device isMultitaskingSupported];
	
	myPhoneViewController = (PhoneViewController*) [myTabBarController.viewControllers objectAtIndex: DIALER_TAB_INDEX];
	
	//Call history
	myCallHistoryTableViewController = [[CallHistoryTableViewController alloc]  initWithNibName:@"CallHistoryTableViewController" bundle:[NSBundle mainBundle]];
	UINavigationController *aCallHistNavigationController = [[UINavigationController alloc] initWithRootViewController:myCallHistoryTableViewController];
	aCallHistNavigationController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:HISTORY_TAB_INDEX] tabBarItem];
	[myCallHistoryTableViewController setPhoneControllerDelegate:myPhoneViewController];
	[myCallHistoryTableViewController setLinphoneDelegate:self];
	

	//people picker delegates
	myContactPickerDelegate = [[ContactPickerDelegate alloc] init];
	myContactPickerDelegate.phoneControllerDelegate=myPhoneViewController;
	myContactPickerDelegate.linphoneDelegate=self;
	//people picker
	myPeoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
	[myPeoplePickerController setDisplayedProperties:[NSArray arrayWithObject:[NSNumber numberWithInt:kABPersonPhoneProperty]]];
	[myPeoplePickerController setPeoplePickerDelegate:myContactPickerDelegate];
	//copy tab bar item
	myPeoplePickerController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:CONTACTS_TAB_INDEX] tabBarItem]; 
	
	//more tab 
	MoreViewController *moreViewController = [[MoreViewController alloc] initWithNibName:@"MoreViewController" bundle:[NSBundle mainBundle]];
	UINavigationController *aNavigationController = [[UINavigationController alloc] initWithRootViewController:moreViewController];
	//copy tab bar item
	aNavigationController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:MORE_TAB_INDEX] tabBarItem]; 
	

	
	//insert contact controller
	NSMutableArray* newArray = [NSMutableArray arrayWithArray:self.myTabBarController.viewControllers];
	[newArray replaceObjectAtIndex:CONTACTS_TAB_INDEX withObject:myPeoplePickerController];
	[newArray replaceObjectAtIndex:MORE_TAB_INDEX withObject:aNavigationController];
	[newArray replaceObjectAtIndex:HISTORY_TAB_INDEX withObject:aCallHistNavigationController];
	
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	[myTabBarController setViewControllers:newArray animated:NO];
	
	[window addSubview:myTabBarController.view];
	
	[window makeKeyAndVisible];
	
	[self	startlibLinphone];
	
	[myCallHistoryTableViewController setLinphoneCore: myLinphoneCore];

	[myPhoneViewController setLinphoneCore: myLinphoneCore];
	
	[ [UIDevice currentDevice] setProximityMonitoringEnabled:true];
	
	
}
-(void) kickOffNetworkConnection {
	CFWriteStreamRef writeStream;
	CFStreamCreatePairWithSocketToHost(NULL, (CFStringRef)@"linphone.org", 15000, nil, &writeStream);
	CFWriteStreamOpen (writeStream);
	const char* buff="hello";
	CFWriteStreamWrite (writeStream,(const UInt8*)buff,strlen(buff));
	CFWriteStreamClose (writeStream);
	
}
- (void)applicationDidBecomeActive:(UIApplication *)application {
	
	if (isStarted) {
		ms_message("becomming active, make sure we are registered");
		[self doRegister];
	} else {
		isStarted=true;
	}
	if (mReadStream !=nil) {

		//unconnect
		int socket = linphone_core_get_sip_socket(myLinphoneCore);
		struct sockaddr hints;
		memset(&hints,0,sizeof(hints));
		hints.sa_family=AF_UNSPEC;
		connect(socket,&hints,sizeof(hints));
		CFReadStreamClose(mReadStream);
		CFRelease(mReadStream);
		mReadStream=nil;
	}
}

-(void)selectDialerTab {
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
}

- (void)applicationWillTerminate:(UIApplication *)application {
	linphone_core_destroy(myLinphoneCore);
}

- (void)dealloc {
	[window release];
	[myPeoplePickerController release];
	[super dealloc];
}

extern void libmsilbc_init();

/*************
 *lib linphone init method
 */
-(void)startlibLinphone  {
	
	//get default config from bundle
	NSBundle* myBundle = [NSBundle mainBundle];
	NSString* factoryConfig = [myBundle pathForResource:@"linphonerc"ofType:nil] ;
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *confiFileName = [[paths objectAtIndex:0] stringByAppendingString:@"/.linphonerc"];
	;
	//log management	
	isDebug = [[NSUserDefaults standardUserDefaults] boolForKey:@"debugenable_preference"];  
	if (isDebug) {
		//redirect all traces to the iphone log framework
		linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
	}
	else {
		linphone_core_disable_logs();
	}
	
	libmsilbc_init();
	
	/*
	 * Initialize linphone core
	 */
	
	myLinphoneCore = linphone_core_new (&linphonec_vtable
										, [confiFileName cStringUsingEncoding:[NSString defaultCStringEncoding]]
										, [factoryConfig cStringUsingEncoding:[NSString defaultCStringEncoding]]
										,self);

	[ self doLinphoneConfiguration:nil];
	[[NSNotificationCenter defaultCenter]	addObserver:self
											selector:@selector(doLinphoneConfiguration:)
											name:NSUserDefaultsDidChangeNotification object:nil];
	
	
	// start scheduler
	[NSTimer scheduledTimerWithTimeInterval:0.1 
									 target:self 
								   selector:@selector(iterate) 
								   userInfo:nil 
									repeats:YES];
	//init audio session
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];
	 BOOL bAudioInputAvailable= [audioSession inputIsAvailable];
	 
	 if(!bAudioInputAvailable){
		 UIAlertView* error = [[UIAlertView alloc]	initWithTitle:@"No microphone"
													message:@"You need to plug a microphone to your device to use this application." 
													delegate:self 
													cancelButtonTitle:@"Ok" 
													otherButtonTitles:nil ,nil];
		 [error show];
	 }
	
	
	
}
-(void) doLinphoneConfiguration:(NSNotification *)notification {
	ms_message("Configuring Linphone");
	
	isDebug = [[NSUserDefaults standardUserDefaults] boolForKey:@"debugenable_preference"];  
	if (isDebug) {
		//redirect all traces to the iphone log framework
		linphone_core_enable_logs_with_cb((OrtpLogFunc)linphone_iphone_log_handler);
	}
	else {
		linphone_core_disable_logs();
	}
	NSString* transport = [[NSUserDefaults standardUserDefaults] stringForKey:@"transport_preference"];
	
	LCSipTransports transportValue;
	if (transport!=nil) {
		if (linphone_core_get_sip_transports(myLinphoneCore, &transportValue)) {
			ms_error("cannot get current transport");	
		}
		if ([transport isEqualToString:@"tcp"]) {
			if (transportValue.tcp_port == 0) transportValue.tcp_port=transportValue.udp_port;
			transportValue.udp_port=0;
		} else if ([transport isEqualToString:@"udp"]){
			if (transportValue.udp_port == 0) transportValue.udp_port=transportValue.tcp_port;
			transportValue.tcp_port=0;
		} else {
			ms_error("unexpected trasnport [%s]",[transport cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}
		if (linphone_core_set_sip_transports(myLinphoneCore, &transportValue)) {
			ms_error("cannot set transport");	
		}
	}
	
	
	
	//initial state is network off
	linphone_core_set_network_reachable(myLinphoneCore,false);
	// Set audio assets
	NSBundle* myBundle = [NSBundle mainBundle];
	const char*  lRing = [[myBundle pathForResource:@"oldphone-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(myLinphoneCore, lRing );
	const char*  lRingBack = [[myBundle pathForResource:@"ringback"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(myLinphoneCore, lRingBack);
 	
	
	
	
	//configure sip account
	
	//madatory parameters
	
	NSString* username = [[NSUserDefaults standardUserDefaults] stringForKey:@"username_preference"];
	NSString* domain = [[NSUserDefaults standardUserDefaults] stringForKey:@"domain_preference"];
	NSString* accountPassword = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
	bool configCheckDisable = [[NSUserDefaults standardUserDefaults] boolForKey:@"check_config_disable_preference"];
	bool isOutboundProxy= [[NSUserDefaults standardUserDefaults] boolForKey:@"outbound_proxy_preference"];
	
	
	//clear auth info list
	linphone_core_clear_all_auth_info(myLinphoneCore);
	//clear existing proxy config
	linphone_core_clear_proxy_config(myLinphoneCore);
	if (proxyReachability !=nil) {
		SCNetworkReachabilityUnscheduleFromRunLoop(proxyReachability,CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	}
	if (username && [username length] >0 && domain && [domain length]>0) {
		
		
		const char* identity = [[NSString stringWithFormat:@"sip:%@@%@",username,domain] cStringUsingEncoding:[NSString defaultCStringEncoding]];
		const char* password = [accountPassword cStringUsingEncoding:[NSString defaultCStringEncoding]];
		
		NSString* proxyAddress = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
		if ((!proxyAddress | [proxyAddress length] <1 ) && domain) {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",domain] ;
		} else {
			proxyAddress = [NSString stringWithFormat:@"sip:%@",proxyAddress] ;
		}
		
		const char* proxy = [proxyAddress cStringUsingEncoding:[NSString defaultCStringEncoding]];
		
		NSString* prefix = [[NSUserDefaults standardUserDefaults] stringForKey:@"prefix_preference"];
		//possible valid config detected
		LinphoneProxyConfig* proxyCfg;	
		proxyCfg = linphone_proxy_config_new();
		
		// add username password
		LinphoneAddress *from = linphone_address_new(identity);
		LinphoneAuthInfo *info;
		if (from !=0){
			info=linphone_auth_info_new(linphone_address_get_username(from),NULL,password,NULL,NULL);
			linphone_core_add_auth_info(myLinphoneCore,info);
		}
		linphone_address_destroy(from);
		
		// configure proxy entries
		linphone_proxy_config_set_identity(proxyCfg,identity);
		linphone_proxy_config_set_server_addr(proxyCfg,proxy);
		linphone_proxy_config_enable_register(proxyCfg,true);
		
		if (isOutboundProxy)
			linphone_proxy_config_set_route(proxyCfg,proxy);
		
		if ([prefix length]>0) {
			linphone_proxy_config_set_dial_prefix(proxyCfg, [prefix cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}
		linphone_proxy_config_set_dial_escape_plus(proxyCfg,TRUE);
		
		linphone_core_add_proxy_config(myLinphoneCore,proxyCfg);
		//set to default proxy
		linphone_core_set_default_proxy(myLinphoneCore,proxyCfg);
		
		LinphoneAddress* addr=linphone_address_new(linphone_proxy_config_get_addr(proxyCfg));
		proxyReachability=SCNetworkReachabilityCreateWithName(nil, linphone_address_get_domain(addr));
		
		
		
		[self doRegister];
	} else if (configCheckDisable == false) { 		
		UIAlertView* error = [[UIAlertView alloc]	initWithTitle:@"Warning"
														message:@"It seems you have not configured any proxy server from settings" 
													   delegate:self 
											  cancelButtonTitle:@"Continue" 
											  otherButtonTitles:@"Never remind",nil];
		[error show];
		proxyReachability=SCNetworkReachabilityCreateWithName(nil, "linphone.org");
		
	}		
	proxyReachabilityContext.info=self;
	SCNetworkReachabilitySetCallback(proxyReachability, (SCNetworkReachabilityCallBack)networkReachabilityCallBack,&proxyReachabilityContext);
	SCNetworkReachabilityScheduleWithRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	
	//Configure Codecs
	
	PayloadType *pt;
	//get codecs from linphonerc	
	const MSList *audioCodecs=linphone_core_get_audio_codecs(myLinphoneCore);
	const MSList *elem;
	//disable all codecs
	for (elem=audioCodecs;elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		linphone_core_enable_payload_type(myLinphoneCore,pt,FALSE);
	}
	
	//read codecs from setting  bundle and enable them one by one
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex_32k_preference"]) { 		
		if(pt = [self findPayload:@"speex"withRate:32000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	} 
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex_16k_preference"]) { 		
		if(pt = [self findPayload:@"speex"withRate:16000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	} 
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex_8k_preference"]) { 
		if(pt = [self findPayload:@"speex"withRate:8000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm_22k_preference"]) { 
		if(pt = [self findPayload:@"GSM"withRate:22050 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm_11k_preference"]) { 
		if(pt = [self findPayload:@"GSM"withRate:11025 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm_8k_preference"]) {
		if(pt = [self findPayload:@"GSM"withRate:8000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"ilbc_preference"]) {
		if(pt = [self findPayload:@"iLBC"withRate:8000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"pcmu_preference"]) {
		if(pt = [self findPayload:@"PCMU"withRate:8000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"pcma_preference"]) {
		if(pt = [self findPayload:@"PCMA"withRate:8000 from:audioCodecs]) {
			linphone_core_enable_payload_type(myLinphoneCore,pt, TRUE);
		}
	}
	
	isbackgroundModeEnabled = [[NSUserDefaults standardUserDefaults] boolForKey:@"backgroundmode_preference"];
	
}

// no proxy configured alert 
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 1) {
		[[NSUserDefaults standardUserDefaults] setBool:true forKey:@"check_config_disable_preference"];  
	}
}


-(void) newIncomingCall:(NSString*) from {
		//redirect audio to speaker
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
		
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
								 , sizeof (audioRouteOverride)
								 , &audioRouteOverride);
    
//#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 40000
	if (backgroundSupported && [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground) {
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif)
		{
			notif.repeatInterval = 0;
			notif.alertBody =[NSString  stringWithFormat:@" %@ is calling you",from];
			notif.alertAction = @"Answer";
			notif.soundName = @"oldphone-mono-30s.caf";
			
			[[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
		}
	} else 
		
//#endif
	{
	UIActionSheet *actionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:@" %@ is calling you",from]
															 delegate:self cancelButtonTitle:@"Decline" destructiveButtonTitle:@"Answer" otherButtonTitles:nil];
    actionSheet.actionSheetStyle = UIActionSheetStyleDefault;
    [actionSheet showFromTabBar:myTabBarController.tabBar];
    [actionSheet release];
	}
		
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 0 ) {
		linphone_core_accept_call(myLinphoneCore,linphone_core_get_current_call(myLinphoneCore));	
	} else {
		linphone_core_terminate_call (myLinphoneCore,linphone_core_get_current_call(myLinphoneCore));
	}
}

- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notification {
	linphone_core_accept_call(myLinphoneCore,linphone_core_get_current_call(myLinphoneCore));	
}
//scheduling loop
-(void) iterate {
	linphone_core_iterate(myLinphoneCore);
}


-(PayloadType*) findPayload:(NSString*)type withRate:(int)rate from:(const MSList*)list {
	const MSList *elem;
	for(elem=list;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		if ([type isEqualToString:[NSString stringWithCString:payload_type_get_mime(pt) encoding:[NSString defaultCStringEncoding]]] && rate==pt->clock_rate) {
			return pt;
		}
	}
	return nil;
	
}

bool networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void * info) {
	id<LinphoneTabManagerDelegate> linphoneDelegate=info;
	LinphoneCore* lc = [linphoneDelegate getLinphoneCore];  
	bool result = false;
	
	if (lc != nil) {
		if ((flags == 0) | (flags & (kSCNetworkReachabilityFlagsConnectionRequired |kSCNetworkReachabilityFlagsConnectionOnTraffic))) {
			[linphoneDelegate kickOffNetworkConnection];
		}
		if (flags) {
			linphone_core_set_network_reachable(lc,true);
			result = true;
		} else {
			linphone_core_set_network_reachable(lc,false);
			result = false;
		}
	}
	return result;
}
-(LinphoneCore*) getLinphoneCore {
	return myLinphoneCore;
}
-(void) doRegister {
	SCNetworkReachabilityFlags reachabilityFlags;
	SCNetworkReachabilityGetFlags (proxyReachability,&reachabilityFlags);
	networkReachabilityCallBack(proxyReachability,reachabilityFlags,self); 
}

@end
