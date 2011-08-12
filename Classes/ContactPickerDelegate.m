/* ContactPickerDelegate.m
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
#import "ContactPickerDelegate.h"
#import "LinphoneManager.h"

@implementation ContactPickerDelegate


- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker 
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person {
	return true;

}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker 
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person 
								property:(ABPropertyID)property 
							  identifier:(ABMultiValueIdentifier)identifier {
	
	CFTypeRef multiValue = ABRecordCopyValue(person, property);
	CFIndex valueIdx = ABMultiValueGetIndexForIdentifier(multiValue,identifier);
	NSString *phoneNumber = (NSString *)ABMultiValueCopyValueAtIndex(multiValue, valueIdx);
	[[LinphoneManager instance].callDelegate displayDialerFromUI:nil 
														 forUser:phoneNumber 
												 withDisplayName:(NSString*)ABRecordCopyCompositeName(person)];
	return false;
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[[LinphoneManager instance].callDelegate displayDialerFromUI:nil 
														 forUser:nil 
												 withDisplayName:@""];
}

@end
