/* ContactPickerDelegate.h
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
#import <Foundation/Foundation.h>
#import <AddressBookUI/ABPeoplePickerNavigationController.h>
#import "PhoneViewController.h"
#import "linphoneAppDelegate.h"


@interface ContactPickerDelegate : NSObject<ABPeoplePickerNavigationControllerDelegate> {
	id<PhoneViewControllerDelegate> phoneControllerDelegate;
	id<LinphoneTabManagerDelegate> linphoneDelegate;
}

@property (nonatomic, retain) id<PhoneViewControllerDelegate> phoneControllerDelegate;
@property (nonatomic, retain) id<LinphoneTabManagerDelegate> linphoneDelegate;

@end
