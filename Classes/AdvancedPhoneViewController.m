/* AdvancedPhoneViewController.m
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

#import "AdvancedPhoneViewController.h"
#import "IncallViewController.h"

@implementation AdvancedPhoneViewController

- (void)viewDidLoad {
    [super viewDidLoad]; 
	mIncallViewController = [[IncallViewController alloc]  initWithNibName:@"IncallViewController" 
																	bundle:[NSBundle mainBundle]];
}

-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[super displayDialerFromUI:viewCtrl
					   forUser:username
			   withDisplayName:displayName];
	
	[mIncallViewController displayDialerFromUI:viewCtrl
									   forUser:username
							   withDisplayName:displayName];
	
}
-(void) displayCall: (LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	/*[super displayCallInProgressFromUI:viewCtrl
					   forUser:username
			   withDisplayName:displayName];*/
	
	[self presentModalViewController:mIncallViewController animated:true];
	
	[mIncallViewController displayCall:call InProgressFromUI:viewCtrl
									   forUser:username
							   withDisplayName:displayName];
	
}

-(void) displayInCall: (LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
        if (linphone_call_get_dir(call)==LinphoneCallIncoming){
                [self presentModalViewController:mIncallViewController animated:true];
        }

	[super displayInCall:call FromUI:viewCtrl
					   forUser:username
			   withDisplayName:displayName];
	
	[mIncallViewController displayInCall:call FromUI:viewCtrl
									   forUser:username
							   withDisplayName:displayName];
	
} 
-(void) displayStatus:(NSString*) message {
	[super displayStatus:message];
	[mIncallViewController displayStatus:message];
}

-(void) updateUIFromLinphoneState:(UIViewController*) viewCtrl {
    [super updateUIFromLinphoneState:viewCtrl];
    
	[mIncallViewController updateUIFromLinphoneState:viewCtrl];
}

- (void)dealloc {
    [mIncallViewController release];
	[super dealloc];
	
}


@end
