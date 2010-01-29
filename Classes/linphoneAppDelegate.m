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
#import "IncallViewController.h"
#import "AddressBook/ABPerson.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import "osip2/osip.h"
#import "FavoriteTableViewController.h"
#import "MoreViewController.h"
#import "ConsoleViewController.h"
#import "FirstLoginViewController.h"


extern void ms_au_register_card();
extern void linphone_iphone_tunneling_init(const char* ip,unsigned int port,bool isDebug);
extern void linphone_iphone_enable_tunneling(LinphoneCore* lc);
extern void linphone_iphone_disable_tunneling(LinphoneCore* lc);
extern int linphone_iphone_tunneling_isready();


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
	NSString* log = [NSString stringWithCString:message length:strlen(message)]; 
	NSLog(log);
	[ConsoleViewController addLog:log];
}
//status 
void linphone_iphone_display_status(struct _LinphoneCore * lc, const char * message) {
	PhoneViewController* lPhone = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myPhoneViewController;
	[lPhone displayStatus:[NSString stringWithCString:message length:strlen(message)]];
}

void linphone_iphone_show(struct _LinphoneCore * lc) {
	//nop
}
void linphone_iphone_call_received(LinphoneCore *lc, const char *from){
	LinphoneAddress* fromAddr = linphone_address_new(from);
	[((linphoneAppDelegate*) linphone_core_get_user_data(lc)) newIncomingCall:[[NSString alloc] initWithCString:linphone_address_get_username(fromAddr) encoding:[NSString defaultCStringEncoding]]];

	
};
void linphone_iphone_general_state(LinphoneCore *lc, LinphoneGeneralState *gstate) {
	//handle first register case
	FirstLoginViewController* loginView = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myFirstLoginViewController;
	if (loginView) {
		[loginView callStateChange:gstate];
	}
	PhoneViewController* lPhone = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myPhoneViewController;
	[lPhone callStateChange:gstate];
}
void linphone_iphone_calllog_updated(LinphoneCore *lc, LinphoneCallLog *newcl) {
	PhoneViewController* lPhone = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myPhoneViewController;
	[lPhone  callLogUpdated:newcl];

}

void linphone_iphone_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username) {
	UIAlertView* alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Error",nil)
													message:NSLocalizedString(@"Wrong user name or password",nil) 
													delegate:nil 
													cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
													otherButtonTitles:nil];
	[alert show];
	FirstLoginViewController* loginView = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myFirstLoginViewController;
	if (loginView) {
		[loginView authInfoRequested];
	}
};


LinphoneCoreVTable linphone_iphone_vtable = {
.show =(ShowInterfaceCb) linphone_iphone_show,
.inv_recv = linphone_iphone_call_received,
.bye_recv = NULL, 
.notify_recv = NULL,
.new_unknown_subscriber = NULL,
.auth_info_requested = linphone_iphone_auth_info_requested,
.display_status = linphone_iphone_display_status,
.display_message=linphone_iphone_log,
.display_warning=linphone_iphone_log,
.display_url=NULL,
.display_question=(DisplayQuestionCb)NULL,
.text_received=NULL,
.general_state=(GeneralStateChange)linphone_iphone_general_state,
.dtmf_received=NULL
};




@implementation linphoneAppDelegate

@synthesize window;
@synthesize myTabBarController;
@synthesize myPeoplePickerController;
@synthesize myPhoneViewController;
@synthesize myFirstLoginViewController;



- (void)applicationDidFinishLaunching:(UIApplication *)application {    
	
	//1 start main UI
	[self launchMainUi];
	
	//2 add first login view if required
	bool isFirstLoginDone = [[NSUserDefaults standardUserDefaults] boolForKey:@"firstlogindone_preference"]; 
	
	if (!isFirstLoginDone) {
	
		myFirstLoginViewController = [[FirstLoginViewController alloc] 
											   initWithNibName:@"FirstLoginViewController" 
											   bundle:nil]; 
		[window addSubview:myFirstLoginViewController.view];
		[myFirstLoginViewController setMainDelegate:self];
	} 

	[window makeKeyAndVisible];


}
-(void) launchMainUi {
	isDebug = [[NSUserDefaults standardUserDefaults] boolForKey:@"debugenable_preference"]; 
	//as defined in PhoneMainView.xib		
	#define DIALER_TAB_INDEX 2
	#define CONTACTS_TAB_INDEX 3
	#define HISTORY_TAB_INDEX 1
	#define FAVORITE_TAB_INDEX 0	
	#define MORE_TAB_INDEX 4
	

	myPhoneViewController = (PhoneViewController*) [myTabBarController.viewControllers objectAtIndex: DIALER_TAB_INDEX];
	[myPhoneViewController  setLinphoneDelegate:self];
	
	myCallHistoryTableViewController = (CallHistoryTableViewController*)[myTabBarController.viewControllers objectAtIndex: HISTORY_TAB_INDEX];
	[myCallHistoryTableViewController setPhoneControllerDelegate:myPhoneViewController];
	[myCallHistoryTableViewController setLinphoneDelegate:self];
	
	myFavoriteTableViewController = (FavoriteTableViewController*)[myTabBarController.viewControllers objectAtIndex: FAVORITE_TAB_INDEX];
	[myFavoriteTableViewController setPhoneControllerDelegate:myPhoneViewController];
	[myFavoriteTableViewController setLinphoneDelegate:self];
	
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
	if (isDebug) {
		[moreViewController  enableLogView];
	}
	[moreViewController setLinphoneDelegate:self];
	UINavigationController *aNavigationController = [[UINavigationController alloc] initWithRootViewController:moreViewController];
	[aNavigationController.navigationBar setTintColor:[UIColor lightGrayColor]];
	//copy tab bar item
	aNavigationController.tabBarItem = [(UIViewController*)[myTabBarController.viewControllers objectAtIndex:MORE_TAB_INDEX] tabBarItem]; 
	
	//insert contact controller and more tab controller
	NSMutableArray* newArray = [NSMutableArray arrayWithArray:self.myTabBarController.viewControllers];
	[newArray replaceObjectAtIndex:CONTACTS_TAB_INDEX withObject:myPeoplePickerController];
	[newArray replaceObjectAtIndex:MORE_TAB_INDEX withObject:aNavigationController];
	
	
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	[myTabBarController setViewControllers:newArray animated:NO];
	
	[window addSubview:myTabBarController.view];
	
	//if (window.keyWindow == NO) [window makeKeyAndVisible];
	
	[self	startlibLinphone];
	
	[myCallHistoryTableViewController setLinphoneCore: myLinphoneCore];
	[myFavoriteTableViewController setLinphoneCore: myLinphoneCore];
	[myPhoneViewController setLinphoneCore: myLinphoneCore];
	[myPhoneViewController setTunnelState:isTunnel];
}

-(void)selectDialerTab {
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
}

- (void)applicationWillTerminate:(UIApplication *)application {
	if (myLinphoneCore) {
		linphone_core_clear_proxy_config(myLinphoneCore);
		linphone_core_destroy(myLinphoneCore);
	}
	
}

- (void)dealloc {
	[window release];
	[myPeoplePickerController release];
	[super dealloc];
}

/*************
 *lib linphone init method
 */
-(void)startlibLinphone  {
	isTunnel=false;
	//init audio session
	NSError *setError = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayAndRecord error: &setError]; //must be call before linphone_core_init
	
	//get default config from bundle
	NSBundle* myBundle = [NSBundle mainBundle];
	NSString* factoryConfigFile = [myBundle pathForResource:@"linphonerc"ofType:nil] ;
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *confiFileName = [[paths objectAtIndex:0] stringByAppendingString:@"/.linphonerc"];

	//log management	
	if (isDebug) {
		//redirect all traces to the iphone log framework
		linphone_core_enable_logs_with_cb(linphone_iphone_log_handler);
	}
	else {
		linphone_core_disable_logs();
	}
	
	//register audio queue sound card
	ms_au_register_card();
	
	/*
	 * Initialize linphone core
	 */
	
	myLinphoneCore = linphone_core_new (&linphone_iphone_vtable, 
										[confiFileName cStringUsingEncoding:[NSString defaultCStringEncoding]],
										[factoryConfigFile cStringUsingEncoding:[NSString defaultCStringEncoding]],
										self);

	
	// Set audio assets
	const char*  lRing = [[myBundle pathForResource:@"oldphone-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(myLinphoneCore, lRing );
	const char*  lRingBack = [[myBundle pathForResource:@"ringback"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(myLinphoneCore, lRingBack);
	
	//init proxy config if not first login
	bool isFirstLoginDone = [[NSUserDefaults standardUserDefaults] boolForKey:@"firstlogindone_preference"]; 
	if (isFirstLoginDone) {
		[self initProxyAndTunnelSettings];
	}
	//Configure Codecs
	
	PayloadType *pt;
	//get codecs from linphonerc	
	const MSList *audioCodecs=linphone_core_get_audio_codecs(myLinphoneCore);
	
	//read from setting  bundle
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex_32k_preference"]) { 		
		if(pt = [self findPayload:@"speex"withRate:32000 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	} 
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex_16k_preference"]) { 		
		if(pt = [self findPayload:@"speex"withRate:16000 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	} 
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex_8k_preference"]) { 
		if(pt = [self findPayload:@"speex"withRate:8000 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm_22k_preference"]) { 
		if(pt = [self findPayload:@"GSM"withRate:22050 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm_11k_preference"]) { 
		if(pt = [self findPayload:@"GSM"withRate:11025 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm_8k_preference"]) {
		if(pt = [self findPayload:@"GSM"withRate:8000 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"pcmu_preference"]) {
		if(pt = [self findPayload:@"PCMU"withRate:8000 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"pcma_preference"]) {
		if(pt = [self findPayload:@"PCMA"withRate:8000 from:audioCodecs]) {
			payload_type_set_enable(pt,TRUE);
		}
	}
	
	
	// start scheduler
	[NSTimer scheduledTimerWithTimeInterval:0.1 
									 target:self 
								   selector:@selector(iterate) 
								   userInfo:nil 
									repeats:YES];
	
}
-(bool)initProxyAndTunnelSettings {
	//tunneling config
	isTunnelConfigured = [[NSUserDefaults standardUserDefaults] boolForKey:@"tunnelenable_preference"]; 
	NSString* username = [[NSUserDefaults standardUserDefaults] stringForKey:@"username_preference"];
	NSString* axtelPin = [[NSUserDefaults standardUserDefaults] stringForKey:@"axtelpin_preference"];
	
	if (isTunnelConfigured) {
		const char* tunnelIp=axtunnel_get_ip_from_key([username cStringUsingEncoding:[NSString defaultCStringEncoding]] 
													  ,[axtelPin cStringUsingEncoding:[NSString defaultCStringEncoding]] );
		if(!tunnelIp) {
			UIAlertView* alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
															message:NSLocalizedString(@"Wrong axtel number or pin, disabling tunnel",nil) 
														   delegate:nil 
												  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
												  otherButtonTitles:nil];
			[alert show];
			isTunnelConfigured=false;
			isTunnel=false;
			
		} else {
			linphone_iphone_tunneling_init(tunnelIp,443,isDebug);
			isTunnel=true;
		}
	}
	
	if (isTunnel) {
		linphone_iphone_enable_tunneling(myLinphoneCore); 
	}
	
	[myPhoneViewController setTunnelState:isTunnel];
	
	//configure sip account
	//get data from Settings bundle
	NSString* accountNameUri = [[NSUserDefaults standardUserDefaults] stringForKey:@"account_preference"];
	NSString* domain = [[NSUserDefaults standardUserDefaults] stringForKey:@"domain_preference"];
	if (!accountNameUri) {
		accountNameUri = [NSString stringWithFormat:@"sip:%@@%@",username,domain];
	}
	const char* identity = [accountNameUri cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	
	NSString* accountPassword = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
	const char* password = [accountPassword cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	NSString* proxyAddress = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	if ((!proxyAddress | [proxyAddress length] <1 ) && domain) {
		proxyAddress = [NSString stringWithFormat:@"sip:%@",domain] ;
	} else {
		proxyAddress = [NSString stringWithFormat:@"sip:%@",proxyAddress] ;
	}
	const char* proxy = [proxyAddress cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	NSString* routeUri = [[NSUserDefaults standardUserDefaults] stringForKey:@"route_preference"];
	const char* route = [routeUri cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	NSString* prefix = [[NSUserDefaults standardUserDefaults] stringForKey:@"prefix_preference"];
	
	if (([accountNameUri length] + [proxyAddress length]) >8 ) {
		//possible valid config detected
		LinphoneProxyConfig* proxyCfg;	
		//clear auth info list
		linphone_core_clear_all_auth_info(myLinphoneCore);
		//clear existing proxy config
		linphone_core_clear_proxy_config(myLinphoneCore);
		proxyCfg = linphone_proxy_config_new();
		
		// add username password
		osip_from_t *from;
		LinphoneAuthInfo *info;
		osip_from_init(&from);
		if (osip_from_parse(from,identity)==0){
			info=linphone_auth_info_new(from->url->username,NULL,password,NULL,NULL);
			linphone_core_add_auth_info(myLinphoneCore,info);
		}
		osip_from_free(from);
		
		// configure proxy entries
		linphone_proxy_config_set_identity(proxyCfg,identity);
		linphone_proxy_config_set_server_addr(proxyCfg,proxy);
		if ([routeUri length] > 4) {
			linphone_proxy_config_set_route(proxyCfg,route);
		}
		LinphoneAddress* addr=linphone_address_new(linphone_proxy_config_get_addr(proxyCfg));
		proxyReachability=SCNetworkReachabilityCreateWithName(nil, linphone_address_get_domain(addr));
		proxyReachabilityContext.info=myLinphoneCore;
		bool result=SCNetworkReachabilitySetCallback(proxyReachability, networkReachabilityCallBack,&proxyReachabilityContext);
		SCNetworkReachabilityFlags reachabilityFlags;
		result=SCNetworkReachabilityGetFlags (proxyReachability,&reachabilityFlags);
		SCNetworkReachabilityScheduleWithRunLoop(proxyReachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		
		if ([prefix length]>0) {
			linphone_proxy_config_set_dial_prefix(proxyCfg, [prefix cStringUsingEncoding:[NSString defaultCStringEncoding]]);
		}
		linphone_proxy_config_set_dial_escape_plus(proxyCfg,TRUE);

		linphone_core_add_proxy_config(myLinphoneCore,proxyCfg);
		//set to default proxy
		linphone_core_set_default_proxy(myLinphoneCore,proxyCfg);
		networkReachabilityCallBack(proxyReachability,reachabilityFlags,myLinphoneCore); 
	}
}

-(void) newIncomingCall:(NSString*) from {
		//redirect audio to speaker
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
		
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
								 , sizeof (audioRouteOverride)
								 , &audioRouteOverride);
    
	UIActionSheet *actionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@"%@ está llamando",nil),from]
																delegate:self cancelButtonTitle:NSLocalizedString(@"Rechazar",nil)
																destructiveButtonTitle:NSLocalizedString(@"Aceptar",nil) otherButtonTitles:nil];
    actionSheet.actionSheetStyle = UIActionSheetStyleDefault;
    [actionSheet showFromTabBar:myTabBarController.tabBar];
    [actionSheet release];
		
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	UInt32 audioRouteNone =  kAudioSessionOverrideAudioRoute_None;  
	
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
							 , sizeof (audioRouteNone)
							 , &audioRouteNone);
	
	if (buttonIndex == 0 ) {
		linphone_core_accept_call(myLinphoneCore,NULL);	
	} else {
		linphone_core_terminate_call (myLinphoneCore,NULL);
	}
}
//scheduling loop
-(void) iterate {
	if (isTunnel==true) {
		if (linphone_iphone_tunneling_isready()  ) {
			linphone_core_iterate(myLinphoneCore);
		}
	}else {
		linphone_core_iterate(myLinphoneCore);
	}
}


-(PayloadType*) findPayload:(NSString*)type withRate:(int)rate from:(const MSList*)list {
	const MSList *elem;
	for(elem=list;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		if ([type isEqualToString:[NSString stringWithCString:payload_type_get_mime(pt) length:strlen(payload_type_get_mime(pt))]] && rate==pt->clock_rate) {
			return pt;
		}
	}
	return nil;
	
}
bool networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void * info) {
	LinphoneProxyConfig* proxyCfg;
	linphone_core_get_default_proxy((LinphoneCore*)info,&proxyCfg);
	linphone_proxy_config_edit(proxyCfg);
	bool result = false;
#ifdef LINPHONE_WIFI_ONLY
	if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == 0) {
		linphone_proxy_config_enable_register(proxyCfg,TRUE);
		result=true;
	} else {
		linphone_proxy_config_enable_register(proxyCfg,FALSE);
		result = false;
	}
#else
	if (flags) {
		// register whatever connection type
		linphone_proxy_config_enable_register(proxyCfg,TRUE);
		result = true;
	} else {
		linphone_proxy_config_enable_register(proxyCfg,false);
		result = false;
	}
#endif
	linphone_proxy_config_done(proxyCfg);
	return result;
}

-(bool) toggleTunnel {
	if (isTunnelConfigured) {
		if (isTunnel) {
			[self disableTunnel];
		} else {
			[self enableTunnel];
		}
		isTunnel=!isTunnel;
	} else {
		UIAlertView* alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
														message:NSLocalizedString(@"Auroc cannot be activated, go to the settings to configure",nil) 
														delegate:nil 
														cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
														otherButtonTitles:nil];
		[alert show];
		
	}
	return isTunnel;
}

-(void) enableTunnel {
	linphone_iphone_enable_tunneling(myLinphoneCore);
	[self doRegister];

}
-(void) disableTunnel {
	linphone_iphone_disable_tunneling(myLinphoneCore);
	[self doRegister];
}

-(void) doRegister {
	SCNetworkReachabilityFlags reachabilityFlags;
	SCNetworkReachabilityGetFlags (proxyReachability,&reachabilityFlags);
	networkReachabilityCallBack(proxyReachability,reachabilityFlags,myLinphoneCore); 
}
-(LinphoneCore*) getLinphoneCore {
	return myLinphoneCore;
}

-(bool) isTunnel {
	return isTunnel;
}

-(void) resetConfig {
	
	[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"username_preference"];
	[[NSUserDefaults standardUserDefaults]removeObjectForKey:@"password_preference"];
	[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"domain_preference"];
	[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"proxy_preference"];

	[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"axtelpin_preference"];
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"tunnelenable_preference"];
	
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"gsm_22k_preference"];
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"gsm_11k_preference"];
	[[NSUserDefaults standardUserDefaults] setBool:true forKey:@"gsm_8k_preference"];
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"pcmu_preference"];
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"pcma_preference"];

	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"debugenable_preference"];
	[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"prefix_preference"];
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"firstlogindone_preference"];
	
}
@end
