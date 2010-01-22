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



@implementation linphoneAppDelegate

@synthesize window;
@synthesize myViewController;

- (void)applicationDidFinishLaunching:(UIApplication *)application {    
	
	PhoneViewController *aViewController = [[PhoneViewController alloc]
										 initWithNibName:@"PhoneViewController" bundle:[NSBundle mainBundle]];
	
	[self  setMyViewController:aViewController];
	[aViewController release];

	//offset the status bar
	CGRect rect = myViewController.view.frame;
	rect = CGRectOffset(rect, 0.0, 20.0);
	myViewController.view.frame = rect;
	
	[window addSubview:[myViewController view]];

	//init lib linphone
	[aViewController startlibLinphone] ;

    [window makeKeyAndVisible];

	
}


- (void)dealloc {
    [window release];
	[myViewController release];
    [super dealloc];
}


@end
