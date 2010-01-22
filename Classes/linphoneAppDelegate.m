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
extern void ms_au_register_card();
//generic log handler for debug version
void linphone_iphone_log_handler(OrtpLogLevel lev, const char *fmt, va_list args){
	NSString* format = [[NSString alloc] initWithCString:fmt encoding:[NSString defaultCStringEncoding]];
	NSLogv(format,args);
	[format release];
}

//Error/warning log handler 
void linphone_iphone_log(struct _LinphoneCore * lc, const char * message) {
	NSLog([NSString stringWithCString:message length:strlen(message)]);
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
	[((linphoneAppDelegate*) linphone_core_get_user_data(lc)) newIncomingCall:[[NSString alloc] initWithCString:from encoding:[NSString defaultCStringEncoding]]];

	
};
void linphone_iphone_general_state(LinphoneCore *lc, LinphoneGeneralState *gstate) {
	PhoneViewController* lPhone = ((linphoneAppDelegate*) linphone_core_get_user_data(lc)).myPhoneViewController;
	[lPhone callStateChange:gstate];
}

LinphoneCoreVTable linphonec_vtable = {
.show =(ShowInterfaceCb) linphone_iphone_show,
.inv_recv = linphone_iphone_call_received,
.bye_recv = NULL, 
.notify_recv = NULL,
.new_unknown_subscriber = NULL,
.auth_info_requested = NULL,
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



- (void)applicationDidFinishLaunching:(UIApplication *)application {    
	
	//as defined in PhoneMainView.xib		
#define DIALER_TAB_INDEX 2
#define CONTACTS_TAB_INDEX 3
#define HISTORY_TAB_INDEX 1
#define FAVORITE_TAB_INDEX 0	
#define MORE_TAB_INDEX 4
	
	
	myPhoneViewController = (PhoneViewController*) [myTabBarController.viewControllers objectAtIndex: DIALER_TAB_INDEX];
	
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
	//insert contact controller
	NSMutableArray* newArray = [NSMutableArray arrayWithArray:self.myTabBarController.viewControllers];
	[newArray replaceObjectAtIndex:CONTACTS_TAB_INDEX withObject:myPeoplePickerController];
	
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	[myTabBarController setViewControllers:newArray animated:NO];
	
	[window addSubview:myTabBarController.view];
	
	[window makeKeyAndVisible];
	
	[self	startlibLinphone];
	
	[myCallHistoryTableViewController setLinphoneCore: myLinphoneCore];
	[myFavoriteTableViewController setLinphoneCore: myLinphoneCore];
	[myPhoneViewController setLinphoneCore: myLinphoneCore];
	
	
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

/*************
 *lib linphone init method
 */
-(void)startlibLinphone  {
	
	//init audio session
	NSError *setError = nil;
	[[AVAudioSession sharedInstance] setCategory: AVAudioSessionCategoryPlayAndRecord error: &setError]; //must be call before linphone_core_init
	
	//get default config from bundle
	NSBundle* myBundle = [NSBundle mainBundle];
	NSString* defaultConfigFile = [myBundle pathForResource:@"linphonerc"ofType:nil] ;
#if TARGET_IPHONE_SIMULATOR
	NSDictionary *dictionary = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:NSFileImmutable];
	[[NSFileManager defaultManager] setAttributes:dictionary ofItemAtPath:defaultConfigFile error:nil];
#endif
	//log management	
	traceLevel = 9;	 
	if (traceLevel > 0) {
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
	
	myLinphoneCore = linphone_core_new (&linphonec_vtable, [defaultConfigFile cStringUsingEncoding:[NSString defaultCStringEncoding]],self);
	
	// Set audio assets
	const char*  lRing = [[myBundle pathForResource:@"oldphone-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(myLinphoneCore, lRing );
	const char*  lRingBack = [[myBundle pathForResource:@"ringback"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(myLinphoneCore, lRingBack);
	
	
	//configure sip account
	//get data from Settings bundle
	NSString* accountNameUri = [[NSUserDefaults standardUserDefaults] stringForKey:@"account_preference"];
	const char* identity = [accountNameUri cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	NSString* accountPassword = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
	const char* password = [accountPassword cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	NSString* proxyUri = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	const char* proxy = [proxyUri cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	NSString* routeUri = [[NSUserDefaults standardUserDefaults] stringForKey:@"route_preference"];
	const char* route = [routeUri cStringUsingEncoding:[NSString defaultCStringEncoding]];
	
	if (([accountNameUri length] + [proxyUri length]) >8 ) {
		//possible valid config detected
		LinphoneProxyConfig* proxyCfg;	
		//clear auth info list
		linphone_core_clear_all_auth_info(myLinphoneCore);
		//get default proxy
		linphone_core_get_default_proxy(myLinphoneCore,&proxyCfg);
		boolean_t addProxy=false;
		if (proxyCfg == NULL) {
			//create new proxy	
			proxyCfg = linphone_proxy_config_new();
			addProxy = true;
		} else {
			linphone_proxy_config_edit(proxyCfg);
		}
		
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
		linphone_proxy_config_enable_register(proxyCfg,TRUE);
		if (addProxy) {
			linphone_core_add_proxy_config(myLinphoneCore,proxyCfg);
			//set to default proxy
			linphone_core_set_default_proxy(myLinphoneCore,proxyCfg);
		} else {
			linphone_proxy_config_done(proxyCfg);
		}
		
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

-(void) newIncomingCall:(NSString*) from {
		//redirect audio to speaker
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
		
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
								 , sizeof (audioRouteOverride)
								 , &audioRouteOverride);
    
	UIActionSheet *actionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:@" %@ is calling you",from]
															 delegate:self cancelButtonTitle:@"Decline" destructiveButtonTitle:@"Answer" otherButtonTitles:nil];
    actionSheet.actionSheetStyle = UIActionSheetStyleDefault;
    [actionSheet showFromTabBar:myTabBarController.tabBar];
    [actionSheet release];
		
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 0 ) {
		linphone_core_accept_call(myLinphoneCore,NULL);	
	} else {
		linphone_core_terminate_call (myLinphoneCore,NULL);
	}
}
//scheduling loop
-(void) iterate {
	linphone_core_iterate(myLinphoneCore);
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


@end
