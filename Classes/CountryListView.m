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

#import "CountryListView.h"
#import "linphone/linphonecore_utils.h"

@interface CountryListView ()

@property (strong, nonatomic) IBOutlet UITableView *tableView;
@property (strong, nonatomic) NSArray *searchResults;

@end

@implementation CountryListView

static NSMutableArray * dataRows = nil;

+ (NSArray*) getData {
	if (!dataRows) {
		dataRows = [[NSMutableArray alloc] init];
		const bctbx_list_t *dialPlans = linphone_dial_plan_get_all_list();
		while (dialPlans) {
			LinphoneDialPlan* dial_plan = (LinphoneDialPlan*)dialPlans->data;
			[dataRows addObject:@{
								  @"name":[NSString stringWithUTF8String:linphone_dial_plan_get_country(dial_plan)],
								  @"iso":[NSString stringWithUTF8String:linphone_dial_plan_get_iso_country_code(dial_plan)],
								  @"code":[NSString stringWithFormat:@"+%s",linphone_dial_plan_get_country_calling_code(dial_plan)],
								  @"phone_length":@(linphone_dial_plan_get_national_number_length(dial_plan))
								  }];
			dialPlans = dialPlans->next;
		}
	}
	return dataRows;
}
#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:nil];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - Other

- (void)viewDidLoad {
    [super viewDidLoad];

    _searchResults = [[NSArray alloc] init];
	self.searchController = [[UISearchController alloc] initWithSearchResultsController:nil];
	self.searchController.searchResultsUpdater = self;
	self.searchController.searchBar.delegate = self;
	self.searchController.obscuresBackgroundDuringPresentation = false;
	[self.searchController.searchBar sizeToFit];
	self.tableView.tableHeaderView = self.searchController.searchBar;
    [_tableView reloadData];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];
  if (self.searchController.active) {
    self.searchController.active = NO;
    [self.searchController.searchBar removeFromSuperview];
  }
}
#pragma mark - UITableView Datasource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (self.searchController.active){
        return _searchResults.count;
    }else{
        return [self.class getData].count;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *cellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:cellIdentifier];
    if(cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:cellIdentifier];
    }

    if (self.searchController.active) {
        cell.textLabel.text = [[_searchResults objectAtIndex:indexPath.row] valueForKey:@"name"];
        cell.detailTextLabel.text = [[_searchResults objectAtIndex:indexPath.row] valueForKey:@"code"];
    }else{
        cell.textLabel.text = [[[self.class getData] objectAtIndex:indexPath.row] valueForKey:@"name"];
        cell.detailTextLabel.text = [[[self.class getData] objectAtIndex:indexPath.row] valueForKey:@"code"];
    }
	return cell;
}

#pragma mark - UITableView Delegate methods

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	if ([_delegate respondsToSelector:@selector(didSelectCountry:)]) {
		NSDictionary* dict = nil;
		if (self.searchController.active) {
			dict = [_searchResults objectAtIndex:indexPath.row];
		}else{
			dict = [[self.class getData] objectAtIndex:indexPath.row];
		}

		[self.delegate didSelectCountry:dict];
	}
	[PhoneMainView.instance popCurrentView];
}

#pragma mark - searchController delegate

- (void)updateSearchResultsForSearchController:(UISearchController *)searchController {
	[self filterContentForSearchText:self.searchController.searchBar.text scope:@""];
	dispatch_async(dispatch_get_main_queue(), ^{
		[self.tableView reloadData];
	});
}

#pragma mark - Filtering

- (void)filterContentForSearchText:(NSString*)searchText scope:(NSString*)scope{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"name contains[c] %@ or code contains %@", searchText, searchText];
    _searchResults = [[self.class getData] filteredArrayUsingPredicate:predicate];
}

- (IBAction)onCancelClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}

+ (NSDictionary *)countryWithIso:(NSString *)iso {
	for (NSDictionary *dict in [self.class getData]) {
		if ([[dict objectForKey:@"iso"] isEqualToString:iso.uppercaseString]) {
			return dict;
		}
	}
	return nil;
}

+ (NSDictionary *)countryWithCountryCode:(NSString *)cc {
	for (NSDictionary *dict in [self.class getData]) {
		if ([[dict objectForKey:@"code"] isEqualToString:cc.uppercaseString]) {
			return dict;
		}
	}
	return nil;
}

@end
