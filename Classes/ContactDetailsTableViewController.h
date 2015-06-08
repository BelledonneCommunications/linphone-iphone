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

#import "ContactDetailsDelegate.h"
#import "ContactDetailsLabelViewController.h"
#import "UIContactDetailsHeader.h"
#import "UIContactDetailsFooter.h"


typedef enum _ContactSections {
    ContactSections_None = 0,
    ContactSections_Number,
    ContactSections_Sip,
    ContactSections_Email,
    ContactSections_MAX
} ContactSections_e;

@interface ContactDetailsTableViewController : UITableViewController<ContactDetailsLabelViewDelegate, UITextFieldDelegate> {
@private
    NSMutableArray *dataCache;
    NSMutableArray *labelArray;
    NSIndexPath *editingIndexPath;
}

@property (nonatomic, assign, setter=setContact:) ABRecordRef contact;
@property (nonatomic, strong) IBOutlet id<ContactDetailsDelegate> contactDetailsDelegate;
@property (nonatomic, strong) IBOutlet UIContactDetailsHeader *headerController;
@property (nonatomic, strong) IBOutlet UIContactDetailsFooter *footerController;

- (BOOL)isValid;
- (void)addPhoneField:(NSString*)number;
- (void)addSipField:(NSString*)address;
- (void)addEmailField:(NSString*)address;
- (void)setContact:(ABRecordRef)contact;

@end
