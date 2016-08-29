//
//  CountryListView.m
//  Country List
//
//  Created by Pradyumna Doddala on 18/12/13.
//  Copyright (c) 2013 Pradyumna Doddala. All rights reserved.
//

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

		for (const LinphoneDialPlan* dial_plan=linphone_dial_plan_get_all(); dial_plan->country!=NULL; dial_plan++) {
			[dataRows addObject:@{
								  @"name":[NSString stringWithUTF8String:dial_plan->country],
								  @"iso":[NSString stringWithUTF8String:dial_plan->iso_country_code],
								  @"code":[NSString stringWithFormat:@"+%s",dial_plan->ccc],
								  @"phone_length":@(dial_plan->nnl)
								  }];
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
    [_tableView reloadData];
}

#pragma mark - UITableView Datasource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (tableView == self.searchDisplayController.searchResultsTableView){
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

    if (tableView == self.searchDisplayController.searchResultsTableView) {
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
		if (tableView == self.searchDisplayController.searchResultsTableView) {
			dict = [_searchResults objectAtIndex:indexPath.row];
		}else{
			dict = [[self.class getData] objectAtIndex:indexPath.row];
		}

		[self.delegate didSelectCountry:dict];
	}
	[PhoneMainView.instance popCurrentView];
}

#pragma mark - Filtering

- (void)filterContentForSearchText:(NSString*)searchText scope:(NSString*)scope{
    NSPredicate *predicate = [NSPredicate predicateWithFormat:@"name contains[c] %@ or code contains %@", searchText, searchText];
    _searchResults = [[self.class getData] filteredArrayUsingPredicate:predicate];
}

- (IBAction)onCancelClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}

- (BOOL)searchDisplayController:(UISearchDisplayController *)controller shouldReloadTableForSearchString:(NSString *)searchString{
    [self filterContentForSearchText:searchString scope:[[self.searchDisplayController.searchBar scopeButtonTitles] objectAtIndex:[self.searchDisplayController.searchBar selectedScopeButtonIndex]]];
        return YES;
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
