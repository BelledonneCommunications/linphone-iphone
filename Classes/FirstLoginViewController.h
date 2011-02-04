/* IncallViewController.h
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
#import "LinphoneAppDelegate.h"
#import "LinphoneUIDelegates.h"


@interface FirstLoginViewController : UIViewController <UITextFieldDelegate,LinphoneUIRegistrationDelegate>{
	UIButton* ok;
	UIButton* site;
	UITextField* username;
	UIView* activityIndicator;
	
	
}
-(void) doOk:(id)sender;

@property (nonatomic, retain) IBOutlet UIButton* ok;
@property (nonatomic, retain) IBOutlet UIButton* site;
@property (nonatomic, retain) IBOutlet UITextField* username;
@property (nonatomic, retain) IBOutlet UITextField* passwd;
@property (nonatomic, retain) IBOutlet UIView* activityIndicator;
@end
