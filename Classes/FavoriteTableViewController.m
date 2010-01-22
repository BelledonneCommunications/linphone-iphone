/* FavoriteTableViewController.h
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

#import "FavoriteTableViewController.h"



@implementation FavoriteTableViewController
@synthesize add;
@synthesize edit;
/*
- (id)initWithStyle:(UITableViewStyle)style {
    // Override initWithStyle: if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
    if (self = [super initWithStyle:style]) {
    }
    return self;
}
*/

/*
- (void)viewDidLoad {
    [super viewDidLoad];

    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}
*/

/*
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
}
*/
/*
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
}
*/
/*
- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
}
*/
/*
- (void)viewDidDisappear:(BOOL)animated {
	[super viewDidDisappear:animated];
}
*/

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (IBAction)doAddFavorite:(id)sender {
	ABPeoplePickerNavigationController* peoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
	[peoplePickerController setPeoplePickerDelegate:self];
	
	[self presentModalViewController: peoplePickerController animated:true];
}
- (IBAction)doEditFavorite:(id)sender {
	if ([ self tableView:self.tableView numberOfRowsInSection:0] <= 0) {
		return;
	}
	if (self.tableView.editing) {
		[self.tableView setEditing:false animated:true];
		[self.edit setTitle:@"Edit" forState: UIControlStateNormal];
	} else {
		[self.tableView setEditing:true animated:true];
		[self.edit setTitle:@"Ok" forState: UIControlStateNormal];
	}
}
#pragma mark Table view methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}


// Customize the number of rows in the table view.
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    const MSList * friends =linphone_core_get_friend_list(myLinphoneCore);
	return ms_list_size(friends);
}


// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    static NSString *CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
    }
    LinphoneFriend* friend = ms_list_nth_data(linphone_core_get_friend_list(myLinphoneCore), indexPath.row);
    const char* name = linphone_address_get_username(linphone_friend_get_uri(friend));
	
	[cell.textLabel setText:[[NSString alloc] initWithCString:name encoding:[NSString defaultCStringEncoding]]];
	cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
	
    return cell;
}
/*
- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath {
	if (isEditing) {
	} else {
		[super tableView:tableView accessoryButtonTappedForRowWithIndexPath:indexPath];
	}
	
}
*/
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // Navigation logic may go here. Create and push another view controller.
	// AnotherViewController *anotherViewController = [[AnotherViewController alloc] initWithNibName:@"AnotherView" bundle:nil];
	// [self.navigationController pushViewController:anotherViewController];
	// [anotherViewController release];
    LinphoneFriend* friend = ms_list_nth_data(linphone_core_get_friend_list(myLinphoneCore), indexPath.row);
    const char* name = linphone_address_get_username(linphone_friend_get_uri(friend));
	[phoneControllerDelegate setPhoneNumber:[[NSString alloc] initWithCString:name encoding:[NSString defaultCStringEncoding]]];
	[linphoneDelegate selectDialerTab];
	
	
}


/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/



// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
		LinphoneFriend* friend = ms_list_nth_data(linphone_core_get_friend_list(myLinphoneCore), indexPath.row);
		linphone_core_remove_friend(myLinphoneCore, friend);
		// Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
		if ([ self tableView:self.tableView numberOfRowsInSection:0] <= 0) {
			[self.tableView setEditing:false animated:true];
			[self.edit setTitle:@"Edit" forState: UIControlStateNormal];
		}
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}



/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/


/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/


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
	NSString *phone = (NSString *)ABMultiValueCopyValueAtIndex(multiValue, valueIdx);
	NSString *phoneUri = [NSString stringWithFormat:@"sip:%s@dummy.net",[phone cStringUsingEncoding:[NSString defaultCStringEncoding]]];
	NSString* compositeName = ABRecordCopyCompositeName(person);
	
	LinphoneFriend * newFriend = linphone_friend_new_with_addr([phoneUri cStringUsingEncoding:[NSString defaultCStringEncoding]]);
	
	linphone_friend_set_name(newFriend,[compositeName cStringUsingEncoding:[NSString defaultCStringEncoding]]);
	linphone_friend_send_subscribe(newFriend, false);
	//linphone_friend_set_sip_addr(newFriend, const char *uri);
	linphone_core_add_friend(myLinphoneCore, newFriend);
	[self dismissModalViewControllerAnimated:true];
	return false;
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[self dismissModalViewControllerAnimated:true];
}

- (void)dealloc {
    [super dealloc];
}


@end

