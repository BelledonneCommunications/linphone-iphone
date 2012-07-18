/* UIContactDetailsHeader.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import "UIContactDetailsHeader.h"
#import "Utils.h"
#import "UIEditableTableViewCell.h"
#import "FastAddressBook.h"

@implementation UIContactDetailsHeader

@synthesize avatarImage;
@synthesize addressLabel;
@synthesize contact;
@synthesize normalView;
@synthesize editView;
@synthesize tableView;

#pragma mark - Lifecycle Functions

- (id)init {
    self = [super initWithNibName:@"UIContactDetailsHeader" bundle:[NSBundle mainBundle]];
    if(self != nil) {
        self->propertyList = [[NSArray alloc] initWithObjects:
                        [NSNumber numberWithInt:kABPersonFirstNameProperty], 
                        [NSNumber numberWithInt:kABPersonLastNameProperty], nil];
    }
    return self;
}

- (void)dealloc {
    [avatarImage release];
    [addressLabel release];
    [normalView release];
    [editView release];
    [tableView release];
    
    [propertyList release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    [tableView setBackgroundColor:[UIColor clearColor]]; // Can't do it in Xib: issue with ios4
}


#pragma mark - Propery Functions

- (void)setContact:(ABRecordRef)acontact {
    contact = acontact;
    [self update];
}


#pragma mark - 

- (void)update {
    if(contact == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update contact details header: null contact"];
        return;
    }
    
    // Avatar image
    {
        UIImage *image = [FastAddressBook getContactImage:contact thumbnail:true];
        if(image == nil) {
            image = [UIImage imageNamed:@"avatar_unknown_small.png"];
        }
        [avatarImage setImage:image];
    }
    
    // Contact label
    {
        [addressLabel setText:[FastAddressBook getContactDisplayName:contact]];
    }
    
    [tableView reloadData];
}

+ (CGFloat)height:(BOOL)editing {
    if(editing) {
        return 130.0f;
    } else {
        return 80.0f;
    }
}

+ (BOOL)findAndResignFirstResponder:(UIView*)view {
    if (view.isFirstResponder) {
        [view resignFirstResponder];
        return YES;     
    }
    for (UIView *subView in view.subviews) {
        if ([UIContactDetailsHeader findAndResignFirstResponder:subView])
            return YES;
    }
    return NO;
}

- (void)setEditing:(BOOL)aediting animated:(BOOL)animated {
    editing = aediting;
    // Resign keyboard
    if(!editing) {
        [UIContactDetailsHeader findAndResignFirstResponder:[self tableView]];
        [self update];
    } 
    if(animated) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:0.3];
    }
    [tableView setEditing:editing animated:animated];
    if(editing) {
        CGRect frame = [editView frame];
        frame.size.height = [UIContactDetailsHeader height:editing];
        [editView setFrame:frame];
        [normalView setAlpha:0.0f];
    } else {
        CGRect frame = [editView frame];
        frame.size.height = 0;
        [editView setFrame:frame];
        [normalView setAlpha:1.0f]; 
    }
    if(animated) {
        [UIView commitAnimations];
    }
}

- (void)setEditing:(BOOL)aediting {
    [self setEditing:aediting animated:FALSE];
}

- (BOOL)isEditing {
    return editing;
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [propertyList count];
}

- (UITableViewCell *)tableView:(UITableView *)atableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"ContactDetailsHeaderCell";
    UIEditableTableViewCell *cell = [atableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {  
        cell = [[[UIEditableTableViewCell alloc] initWithStyle:UITableViewCellStyleValue2 reuseIdentifier:kCellId] autorelease];
    }
   
    ABPropertyID property = [[propertyList objectAtIndex:[indexPath row]] intValue];
    if(property == kABPersonFirstNameProperty) {
        [cell.detailTextField setPlaceholder:@"First name"];
    } else if (property == kABPersonLastNameProperty) {
        [cell.detailTextField setPlaceholder:@"Last name"];
    }
    [cell.detailTextField setKeyboardType:UIKeyboardTypeDefault];
    if(contact) {
        CFStringRef lValue = ABRecordCopyValue(contact, property);
        if(lValue != NULL) {
            [cell.detailTextLabel setText:(NSString*)lValue];
            [cell.detailTextField setText:(NSString*)lValue];
            CFRelease(lValue);
        } else {
            [cell.detailTextLabel setText:@""];
            [cell.detailTextField setText:@""];
        }
    }
    [cell.detailTextField setDelegate:self];
    
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    return UITableViewCellEditingStyleNone;
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];    
    return YES;
}

- (BOOL)textFieldShouldEndEditing:(UITextField *)textField {
    UIView *view = [textField superview]; 
    // Find TableViewCell
    if(view != nil && ![view isKindOfClass:[UIEditableTableViewCell class]]) view = [view superview];
    if(view != nil) {
        UIEditableTableViewCell *cell = (UIEditableTableViewCell*)view;
        NSIndexPath *indexPath = [self.tableView indexPathForCell:cell];
        ABPropertyID property = [[propertyList objectAtIndex:[indexPath row]] intValue];
        [cell.detailTextLabel setText:[textField text]];
        NSError* error = NULL;
        ABRecordSetValue(contact, property, [textField text], (CFErrorRef*)&error);
        if (error != NULL) {
            [LinphoneLogger log:LinphoneLoggerError format:@"Error when saving property %i in contact %p: Fail(%@)", property, contact, [error localizedDescription]];
        } 
    } else {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Not valid UIEditableTableViewCell"];
    }
    return TRUE;
}

@end
