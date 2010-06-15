/* linphoneAppDelegate.h
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


#import <UIKit/UIKit.h>
#import <AddressBookUI/ABPeoplePickerNavigationController.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#include"linphonecore.h"


@protocol LinphoneTabManagerDelegate

-(void)selectDialerTab;
-(void) kickOffNetworkConnection;
-(LinphoneCore*) getLinphoneCore; 
@end

@class ContactPickerDelegate;
@class IncallViewController;
@class PhoneViewController;
@class CallHistoryTableViewController;

@interface linphoneAppDelegate : NSObject <UIApplicationDelegate,LinphoneTabManagerDelegate,UIActionSheetDelegate,UIAlertViewDelegate> {
    UIWindow *window;
	IBOutlet UITabBarController*  myTabBarController;
	IBOutlet ABPeoplePickerNavigationController* myPeoplePickerController;
	IBOutlet PhoneViewController* myPhoneViewController;
	CallHistoryTableViewController* myCallHistoryTableViewController;
	
	ContactPickerDelegate* myContactPickerDelegate;
	
	bool isDebug;
	bool isStarted;
	bool backgroundSupported;

	LinphoneCore* myLinphoneCore;
	SCNetworkReachabilityContext proxyReachabilityContext;
	SCNetworkReachabilityRef proxyReachability;
	CFReadStreamRef mReadStream;

	
}
/**********************************
 * liblinphone initialization method
 **********************************/
-(void) startlibLinphone;

/*
 * liblinphone scheduling method;
 */
-(void) iterate;

-(void) newIncomingCall:(NSString*) from;


-(PayloadType*) findPayload:(NSString*)type withRate:(int)rate from:(const MSList*)list;

/**
 * return true if register is activated
 */
bool networkReachabilityCallBack(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void * info);

-(void) doRegister;
-(void) doLinphoneConfiguration:(NSNotification *)notification;


@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UITabBarController*  myTabBarController;
@property (nonatomic, retain) ABPeoplePickerNavigationController* myPeoplePickerController;
@property (nonatomic, retain) IBOutlet PhoneViewController* myPhoneViewController;


@end

