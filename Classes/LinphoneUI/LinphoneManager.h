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

#import <Foundation/Foundation.h>
#import <AVFoundation/AVAudioSession.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#include "linphonecore.h"
#import "LogView.h"
#import "LinphoneUIDelegates.h"
typedef enum _Connectivity {
	wifi,
	wwan
	,none
} Connectivity;
@class FastAddressBook;

/* Application specific call context */
typedef struct _CallContext {
    LinphoneCall* call;
    bool_t cameraIsEnabled;
} CallContext;

@interface LinphoneManager : NSObject <AVAudioSessionDelegate> {
@private
	SCNetworkReachabilityContext proxyReachabilityContext;
	SCNetworkReachabilityRef proxyReachability;
	NSTimer* mIterateTimer;
	id<LogView> mLogView;	
	bool isbackgroundModeEnabled;
	id<LinphoneUICallDelegate> callDelegate;
	id<LinphoneUIRegistrationDelegate> registrationDelegate;
	
	UIViewController* mCurrentViewController;
	Connectivity connectivity;
    FastAddressBook* mFastAddressBook;
	const char*  frontCamId;
	const char*  backCamId;
    
@public
    CallContext currentCallContextBeforeGoingBackground;
}
+(LinphoneManager*) instance;
+(LinphoneCore*) getLc;
+(BOOL) runningOnIpad;
+(void) set:(UIView*)view hidden: (BOOL) hidden withName:(const char*)name andReason:(const char*) reason;
+(void) logUIElementPressed:(const char*) name;

-(void) registerLogView:(id<LogView>) view;

-(void) startLibLinphone;
-(BOOL) isNotIphone3G;
-(void) destroyLibLinphone;
  
-(void) enterBackgroundMode;
-(void) becomeActive;
-(void) kickOffNetworkConnection;
-(NSString*) getDisplayNameFromAddressBook:(NSString*) number andUpdateCallLog:(LinphoneCallLog*)log; 
-(UIImage*) getImageFromAddressBook:(NSString*) number;



@property (nonatomic, retain) id<LinphoneUICallDelegate> callDelegate;
@property (nonatomic, retain) id<LinphoneUIRegistrationDelegate> registrationDelegate;
@property Connectivity connectivity;
@property (readonly) const char*  frontCamId;
@property (readonly) const char*  backCamId;
@end


