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
#import <SystemConfiguration/SCNetworkReachability.h>
#include "linphonecore.h"
#import "LogView.h"
#import "LinphoneUIDelegates.h"
typedef enum _Connectivity {
	wifi,
	wwan
	,none
} Connectivity;
typedef enum _TunnelMode {
	off
	,on
	,wwan_only
	,autodetect
} TunnelMode;

@class FastAddressBook;
@interface LinphoneManager : NSObject {
@private
	SCNetworkReachabilityContext proxyReachabilityContext;
	SCNetworkReachabilityRef proxyReachability;
	CFReadStreamRef mReadStream;
	NSTimer* mIterateTimer;
	id<LogView> mLogView;	
	bool isbackgroundModeEnabled;
	id<LinphoneUICallDelegate> callDelegate;
	id<LinphoneUIRegistrationDelegate> registrationDelegate;
	
	UIViewController* mCurrentViewController;
	Connectivity connectivity;
    FastAddressBook* mFastAddressBook;
	TunnelMode tunnelMode;
	
}
+(LinphoneManager*) instance;
+(LinphoneCore*) getLc;

-(void) registerLogView:(id<LogView>) view;

-(void) startLibLinphone;
-(void) destroyLibLinphone;
  
-(void) enterBackgroundMode;
-(void) becomeActive;
-(void) kickOffNetworkConnection;
-(NSString*) getDisplayNameFromAddressBook:(NSString*) number andUpdateCallLog:(LinphoneCallLog*)log; 

@property (nonatomic, retain) id<LinphoneUICallDelegate> callDelegate;
@property (nonatomic, retain) id<LinphoneUIRegistrationDelegate> registrationDelegate;
@property Connectivity connectivity;
@property TunnelMode tunnelMode;
@end


