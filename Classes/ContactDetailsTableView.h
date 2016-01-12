/* ContactDetailsTableViewController.h
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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
#import <AddressBook/AddressBook.h>

#import "Contact.h"
#import "LinphoneUI/UIToggleButton.h"

typedef enum _ContactSections {
	ContactSections_None = 0, // first section is empty because we cannot set header for first section
	ContactSections_FirstName,
	ContactSections_LastName,
	ContactSections_Sip,
	ContactSections_Number,
	ContactSections_Email,
	ContactSections_MAX
} ContactSections;

@interface ContactDetailsTableView : UITableViewController <UITextFieldDelegate>

@property(strong, nonatomic) Contact *contact;
@property(weak, nonatomic) IBOutlet UIToggleButton *editButton;

- (void)addPhoneField:(NSString *)number;
- (void)addSipField:(NSString *)address;
- (void)addEmailField:(NSString *)address;
- (void)setContact:(Contact *)contact;

@end
