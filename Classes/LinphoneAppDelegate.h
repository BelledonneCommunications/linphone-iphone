/* LinphoneAppDelegate.h
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
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */                                                                           

#import <UIKit/UIKit.h>
#import <PushKit/PushKit.h>
#import <AddressBookUI/ABPeoplePickerNavigationController.h>


#import "LinphoneCoreSettingsStore.h"

//@interface LinphoneAppDelegate : NSObject <UIApplicationDelegate,UIAlertViewDelegate> {
@interface LinphoneAppDelegate : NSObject <UIApplicationDelegate,UIAlertViewDelegate, PKPushRegistryDelegate> {
    @private
	UIBackgroundTaskIdentifier bgStartId;
    BOOL startedInBackground;
}

- (void)processRemoteNotification:(NSDictionary*)userInfo;
- (void)registerForNotifications:(UIApplication *)app;

@property (nonatomic, retain) UIAlertView *waitingIndicator;
@property (nonatomic, retain) NSString *configURL;
@property (nonatomic, strong) UIWindow* window;
@property PKPushRegistry* voipRegistry;


@end

