/* PhoneViewController.h
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
#import "osip2/osip.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>


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
	PhoneViewController* lPhone = linphone_core_get_user_data(lc);
	[lPhone.status setText:[NSString stringWithCString:message length:strlen(message)]];
}

void linphone_iphone_show(struct _LinphoneCore * lc) {
	//nop
}
void linphone_iphone_call_received(LinphoneCore *lc, const char *from){
	//redirect audio to speaker
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
	
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
				, sizeof (audioRouteOverride)
				, &audioRouteOverride);
};

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
.general_state=NULL,
.dtmf_received=NULL
};



@implementation PhoneViewController
@synthesize  address ;
@synthesize  call;
@synthesize  cancel;
@synthesize status;

@synthesize one;
@synthesize two;
@synthesize three;
@synthesize four;
@synthesize five;
@synthesize six;
@synthesize seven;
@synthesize eight;
@synthesize nine;
@synthesize star;
@synthesize zero;
@synthesize hash;


//implements call/cancel button behavior 
-(IBAction) doAction:(id)sender {
	
	if (sender == call) {
		if (!linphone_core_in_call(mCore)) {
			const char* lCallee = [[address text]  cStringUsingEncoding:[NSString defaultCStringEncoding]];
			linphone_core_invite(mCore,lCallee) ;		
		} 
		if (linphone_core_inc_invite_pending(mCore)) {
			linphone_core_accept_call(mCore,NULL);	
		}
		//Cancel audio route redirection
		UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
		
		AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
					, sizeof (audioRouteOverride)
					, &audioRouteOverride);
	} else if (sender == cancel) {
		linphone_core_terminate_call(mCore,NULL);
	} 

}

//implements keypad behavior 
-(IBAction) doKeyPad:(id)sender {
	if (linphone_core_in_call(mCore)) {
	//incall behavior
		if (sender == one) {
			linphone_core_send_dtmf(mCore,'1');	
		} else if (sender == two) {
			linphone_core_send_dtmf(mCore,'2');	
		} else if (sender == three) {
			linphone_core_send_dtmf(mCore,'3');	
		} else if (sender == four) {
			linphone_core_send_dtmf(mCore,'4');	
		} else if (sender == five) {
			linphone_core_send_dtmf(mCore,'5');	
		} else if (sender == six) {
			linphone_core_send_dtmf(mCore,'6');	
		} else if (sender == seven) {
			linphone_core_send_dtmf(mCore,'7');	
		} else if (sender == eight) {
			linphone_core_send_dtmf(mCore,'8');	
		} else if (sender == nine) {
			linphone_core_send_dtmf(mCore,'9');	
		} else if (sender == star) {
			linphone_core_send_dtmf(mCore,'*');	
		} else if (sender == zero) {
			linphone_core_send_dtmf(mCore,'0');	
		} else if (sender == hash) {
			linphone_core_send_dtmf(mCore,'#');	
		} else  {
			NSLog(@"unknown event from dial pad");	
		}
	} else {
		//outcall behavior	
		//remove sip: if first digits
		if ([address.text isEqualToString:@"sip:"]) {
			[address setText:@""];
		}
		NSString* newAddress = nil;
		if (sender == one) {
			newAddress = [address.text stringByAppendingString:@"1"];
		} else if (sender == two) {
			newAddress = [address.text stringByAppendingString:@"2"];
		} else if (sender == three) {
			newAddress = [address.text stringByAppendingString:@"3"];
		} else if (sender == four) {
			newAddress = [address.text stringByAppendingString:@"4"];
		} else if (sender == five) {
			newAddress = [address.text stringByAppendingString:@"5"];
		} else if (sender == six) {
			newAddress = [address.text stringByAppendingString:@"6"];
		} else if (sender == seven) {
			newAddress = [address.text stringByAppendingString:@"7"];
		} else if (sender == eight) {
			newAddress = [address.text stringByAppendingString:@"8"];
		} else if (sender == nine) {
			newAddress = [address.text stringByAppendingString:@"9"];
		} else if (sender == star) {
			newAddress = [address.text stringByAppendingString:@"*"];
		} else if (sender == zero) {
			newAddress = [address.text stringByAppendingString:@"0"];
		} else if (sender == hash) {
			newAddress = [address.text stringByAppendingString:@"#"];
		} else  {
			NSLog(@"unknown event from diad pad");	
		}
		[address setText:newAddress];	
	}
}


/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
    }
    
	return self;
}
*/

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}
*/

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    if (theTextField == address) {
        [address resignFirstResponder];
    }
    return YES;
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
	
	mCore = linphone_core_new (&linphonec_vtable, [defaultConfigFile cStringUsingEncoding:[NSString defaultCStringEncoding]],self);
	
	// Set audio assets
	const char*  lRing = [[myBundle pathForResource:@"oldphone-mono"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ring(mCore, lRing );
	const char*  lRingBack = [[myBundle pathForResource:@"ringback"ofType:@"wav"] cStringUsingEncoding:[NSString defaultCStringEncoding]];
	linphone_core_set_ringback(mCore, lRingBack);
	
	
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
		linphone_core_clear_all_auth_info(mCore);
		//get default proxy
		linphone_core_get_default_proxy(mCore,&proxyCfg);
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
                        linphone_core_add_auth_info(mCore,info);
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
			linphone_core_add_proxy_config(mCore,proxyCfg);
			//set to default proxy
			linphone_core_set_default_proxy(mCore,proxyCfg);
		} else {
			linphone_proxy_config_done(proxyCfg);
		}

	}

	//Configure Codecs

	PayloadType *pt;
	//get codecs from linphonerc	
	const MSList *audioCodecs=linphone_core_get_audio_codecs(mCore);
	
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
//scheduling loop
-(void) iterate {
	linphone_core_iterate(mCore);
}


- (void)dealloc {
    [address dealloc];
	[call dealloc];
	[cancel dealloc];
	[status dealloc];
	[super dealloc];
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
