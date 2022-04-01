/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "linphoneapp-Swift.h"
#import "ContactsListTableView.h"
#import "UIContactCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils.h"

@implementation ContactsListTableView

#pragma mark - Lifecycle Functions

- (void)initContactsTableViewController {
	addressBookMap = [[OrderedDictionary alloc] init];
	[[NSNotificationCenter defaultCenter]
	 addObserver:self
	 selector:@selector(onAddressBookUpdate:)
	 name:CNContactStoreDidChangeNotification
	 object:nil];
	[[NSNotificationCenter defaultCenter]
	 addObserver:self
	 selector:@selector(onMagicSearchFinished:)
	 name:kLinphoneMagicSearchFinished
	 object:nil];
}

- (void)onAddressBookUpdate:(NSNotification *)k {
	if ((![MagicSearchSingleton.instance isSearchOngoing] && (PhoneMainView.instance.currentView == ContactsListView.compositeViewDescription)) || (IPAD && PhoneMainView.instance.currentView == ContactDetailsView.compositeViewDescription)) {
		[self loadData];
	}
}

- (void)onMagicSearchFinished:(NSNotification *)k {
	[self buildContactTable];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	if (IPAD) {
		if (![self selectFirstRow]) {
			ContactDetailsView *view = VIEW(ContactDetailsView);
			[view setContact:nil];
		}
	}
}

- (id)init {
	self = [super init];
	if (self) {
		[self initContactsTableViewController];
	}
	_reloadMagicSearch = TRUE;
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initContactsTableViewController];
	}
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self removeAllContacts];
}

- (void)removeAllContacts {
	for (NSInteger j = 0; j < [self.tableView numberOfSections]; ++j) {
		for (NSInteger i = 0; i < [self.tableView numberOfRowsInSection:j]; ++i) {
			[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:j]] setContact:nil];
		}
	}
}

#pragma mark -

static int ms_strcmpfuz(const char *fuzzy_word, const char *sentence) {
	if (!fuzzy_word || !sentence) {
		return fuzzy_word == sentence;
	}
	const char *c = fuzzy_word;
	const char *within_sentence = sentence;
	for (; c != NULL && *c != '\0' && within_sentence != NULL; ++c) {
		within_sentence = strchr(within_sentence, *c);
		// Could not find c character in sentence. Abort.
		if (within_sentence == NULL) {
			break;
		}
		// since strchr returns the index of the matched char, move forward
		within_sentence++;
	}

	// If the whole fuzzy was found, returns 0. Otherwise returns number of characters left.
	return (int)(within_sentence != NULL ? 0 : fuzzy_word + strlen(fuzzy_word) - c);
}

- (NSString *)displayNameForContact:(Contact *)person {
	NSString *name = person.displayName;
	if (name != nil && [name length] > 0 && ![name isEqualToString:NSLocalizedString(@"Unknown", nil)]) {
		
		// Sort contacts by first letter. We need to translate the name to ASCII first, because of UTF-8
		// issues. For instance expected order would be:  Alberta(A tilde) before ASylvano.
		NSData *name2ASCIIdata = [name dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
		NSString *name2ASCII = [[NSString alloc] initWithData:name2ASCIIdata encoding:NSASCIIStringEncoding];
		return name2ASCII;
	}
	return NSLocalizedString(@"Unknown", nil);
}

- (void)buildContactTable {
	@synchronized(addressBookMap) {
		//Set all contacts from ContactCell to nil
		for (NSInteger j = 0; j < [self.tableView numberOfSections]; ++j)
		{
			for (NSInteger i = 0; i < [self.tableView numberOfRowsInSection:j]; ++i)
			{
				[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:j]] setContact:nil];
			}
		}
		// Reset Address book
		[addressBookMap removeAllObjects];
		
		NSMutableArray *subAr = [NSMutableArray new];
		[addressBookMap insertObject:subAr forKey:@"" selector:@selector(caseInsensitiveCompare:)];
		//
		
		NSArray *searchResults = [MagicSearchSingleton.instance getLastSearchContacts];
		
		for (Contact *contact in searchResults) {
			NSMutableString *name = [[NSMutableString alloc] initWithString: [self displayNameForContact:contact]];
			if (name != nil) {
				NSString *firstChar = [[name substringToIndex:1] uppercaseString];
				// Put in correct subAr
				if ([firstChar characterAtIndex:0] < 'A' || [firstChar characterAtIndex:0] > 'Z') {
					firstChar = @"#";
				}
				NSMutableArray *subAr = [addressBookMap objectForKey:firstChar];
				if (subAr == nil) {
					subAr = [[NSMutableArray alloc] init];
					[addressBookMap insertObject:subAr forKey:firstChar selector:@selector(caseInsensitiveCompare:)];
				}
				NSUInteger idx = [subAr indexOfObject:contact inSortedRange:(NSRange){0, subAr.count} options:NSBinarySearchingInsertionIndex usingComparator:^NSComparisonResult( Contact *_Nonnull obj1, Contact *_Nonnull obj2) {
					return [[self displayNameForContact:obj1] compare:[self displayNameForContact:obj2] options:NSCaseInsensitiveSearch];
				}];
				if (![subAr containsObject:contact]) {
					[subAr insertObject:contact atIndex:idx];
				}
			}
		}
		[super loadData];
	}
	// since we refresh the tableview, we must perform this on main
	// thread
	dispatch_async(dispatch_get_main_queue(), ^(void) {
		if (IPAD) {
			if (!([self totalNumberOfItems] > 0)) {
				ContactDetailsView *view = VIEW(ContactDetailsView);
				[view setContact:nil];
			}
		}
	});
	_reloadMagicSearch = FALSE;
}

- (void)loadData {
	if (_reloadMagicSearch) {
		NSString *domain = @"";
		if ([ContactSelection getSipFilterEnabled]) {
			LinphoneAccount *defaultAccount = linphone_core_get_default_account(LC);
			if (defaultAccount) {
				domain = [NSString stringWithUTF8String:linphone_account_params_get_domain(linphone_account_get_params(defaultAccount))];
			}
		}
		int sourceFlags = LinphoneMagicSearchSourceFriends | LinphoneMagicSearchSourceLdapServers;
		[MagicSearchSingleton.instance searchForContactsWithDomain:domain sourceFlags:sourceFlags clearCache:[LinphoneManager.instance getContactsUpdated]];
		[LinphoneManager.instance setContactsUpdated:FALSE];
	} else {
		[self buildContactTable];
	}
}

- (void)loadDataWithFilter: (NSString *)filter {
	LOGI(@"Load search contact list");
	_reloadMagicSearch = _reloadMagicSearch || [filter length]==0 || ![[MagicSearchSingleton.instance currentFilter] isEqualToString:filter];
	[MagicSearchSingleton.instance setCurrentFilter:filter];
	[self loadData];
}

#pragma mark - UITableViewDataSource Functions

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
	return [addressBookMap allKeys];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return [addressBookMap count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [(OrderedDictionary *)[addressBookMap objectForKey:[addressBookMap keyAtIndex:section]] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIContactCell.class);
	UIContactCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIContactCell alloc] initWithIdentifier:kCellId];
	}
	NSMutableArray *subAr = [addressBookMap objectForKey:[addressBookMap keyAtIndex:[indexPath section]]];
	Contact *contact = subAr[indexPath.row];

	// Cached avatar
	UIImage *image = [FastAddressBook imageForContact:contact];
	[cell.avatarImage setImage:image bordered:NO withRoundedRadius:YES];
	[cell setContact:contact];
	[super accessoryForCell:cell atPath:indexPath];
	cell.contentView.userInteractionEnabled = false;

	return cell;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	CGRect frame = CGRectMake(0, 0, tableView.frame.size.width, tableView.sectionHeaderHeight);
	UIView *tempView = [[UIView alloc] initWithFrame:frame];
	if (@available(iOS 13, *)) {
		tempView.backgroundColor = [UIColor systemBackgroundColor];
	} else {
		tempView.backgroundColor = [UIColor whiteColor];
	}

	UILabel *tempLabel = [[UILabel alloc] initWithFrame:frame];
	tempLabel.backgroundColor = [UIColor clearColor];
	tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]];
	tempLabel.text = [addressBookMap keyAtIndex:section];
	tempLabel.textAlignment = NSTextAlignmentCenter;
	tempLabel.font = [UIFont boldSystemFontOfSize:17];
	tempLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
	[tempView addSubview:tempLabel];

	return tempView;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[super tableView:tableView didSelectRowAtIndexPath:indexPath];
	if (![self isEditing]) {
		NSMutableArray *subAr = [addressBookMap objectForKey:[addressBookMap keyAtIndex:[indexPath section]]];
		Contact *contact = subAr[indexPath.row];

		// Go to Contact details view
		ContactDetailsView *view = VIEW(ContactDetailsView);
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
		if (([ContactSelection getSelectionMode] != ContactSelectionModeEdit) || !([ContactSelection getAddAddress])) {
			[view setContact:contact];
		} else {
			if (IPAD) {
				[view resetContact];
				view.isAdding = FALSE;
			}
			[view editContact:contact address:[ContactSelection getAddAddress]];
		}
	}
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[NSNotificationCenter.defaultCenter removeObserver:self];
		
		NSString *msg = NSLocalizedString(@"Do you want to delete selected contact?\nIt will also be deleted from your phone's address book.", nil);
		[UIConfirmationDialog ShowWithMessage:msg
					cancelMessage:nil
					confirmMessage:nil
					onCancelClick:nil
					onConfirmationClick:^() {
						[tableView beginUpdates];

						NSString *firstChar = [addressBookMap keyAtIndex:[indexPath section]];
						NSMutableArray *subAr = [addressBookMap objectForKey:firstChar];
						Contact *contact = subAr[indexPath.row];
						[subAr removeObjectAtIndex:indexPath.row];
						if (subAr.count == 0) {
							[addressBookMap removeObjectForKey:firstChar];
							[tableView deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section]
							withRowAnimation:UITableViewRowAnimationFade];
						}
						UIContactCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
						[cell setContact:NULL];
						[[LinphoneManager.instance fastAddressBook] deleteContact:contact];
						[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
						[tableView endUpdates];

						[NSNotificationCenter.defaultCenter	addObserver:self selector:@selector(onAddressBookUpdate:)
											name:kLinphoneAddressBookUpdate
											object:nil];
						[self loadData];
					}];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
	  [NSNotificationCenter.defaultCenter removeObserver:self];

	  NSString *firstChar = [addressBookMap keyAtIndex:[indexPath section]];
	  NSMutableArray *subAr = [addressBookMap objectForKey:firstChar];
	  Contact *contact = subAr[indexPath.row];
	  [subAr removeObjectAtIndex:indexPath.row];
	  if (subAr.count == 0) {
		  [addressBookMap removeObjectForKey:firstChar];
	  }
	  UIContactCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
	  [cell setContact:NULL];
	  [[LinphoneManager.instance fastAddressBook] deleteContact:contact];

	  [NSNotificationCenter.defaultCenter addObserver:self
                 selector:@selector(onAddressBookUpdate:)
                     name:kLinphoneAddressBookUpdate
                   object:nil];
	}];
}

@end
