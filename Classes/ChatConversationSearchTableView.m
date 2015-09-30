//
//  MyTableViewController.m
//  UISearchDisplayController
//
//  Created by Phillip Harris on 4/19/14.
//  Copyright (c) 2014 Phillip Harris. All rights reserved.
//

#import "ChatConversationSearchTableView.h"

@interface ChatConversationSearchTableView ()

@property(nonatomic, strong) NSArray *names;
@property(nonatomic, strong) NSArray *searchResults;
@property(nonatomic, strong) UISearchDisplayController *searchController;

@end

@implementation ChatConversationSearchTableView

//===============================================
#pragma mark -
#pragma mark Initialization
//===============================================

- (id)initWithStyle:(UITableViewStyle)style {
	self = [super initWithStyle:style];
	if (self) {
		[self commonInit];
	}
	return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];
	if (self) {
		[self commonInit];
	}
	return self;
}

- (void)commonInit {
	_names = @[
		@"Aaliyah",
		@"Aaron",
		@"Abigail",
		@"Adam",
		@"Addison",
		@"Adrian",
		@"Aiden",
		@"Alex",
		@"Alexa",
		@"Alexander",
		@"Alexandra",
		@"Alexis",
		@"Allison",
		@"Alyssa",
		@"Amelia",
		@"Andrea",
		@"Andrew",
		@"Angel",
		@"Anna",
		@"Annabelle",
		@"Anthony",
		@"Aria",
		@"Ariana",
		@"Arianna",
		@"Ashley",
		@"Aubree",
		@"Aubrey",
		@"Audrey",
		@"Austin",
		@"Autumn",
		@"Ava",
		@"Avery",
		@"Ayden",
		@"Bailey",
		@"Bella",
		@"Benjamin",
		@"Bentley",
		@"Blake",
		@"Brandon",
		@"Brayden",
		@"Brianna",
		@"Brody",
		@"Brooklyn",
		@"Bryson",
		@"Caleb",
		@"Cameron",
		@"Camila",
		@"Carlos",
		@"Caroline",
		@"Carson",
		@"Carter",
		@"Charles",
		@"Charlotte",
		@"Chase",
		@"Chloe",
		@"Christian",
		@"Christopher",
		@"Claire",
		@"Colton",
		@"Connor",
		@"Cooper",
		@"Damian",
		@"Daniel",
		@"David",
		@"Dominic",
		@"Dylan",
		@"Easton",
		@"Eli",
		@"Elijah",
		@"Elizabeth",
		@"Ella",
		@"Ellie",
		@"Emily",
		@"Emma",
		@"Ethan",
		@"Eva",
		@"Evan",
		@"Evelyn",
		@"Faith",
		@"Gabriel",
		@"Gabriella",
		@"Gavin",
		@"Genesis",
		@"Gianna",
		@"Grace",
		@"Grayson",
		@"Hailey",
		@"Hannah",
		@"Harper",
		@"Henry",
		@"Hudson",
		@"Hunter",
		@"Ian",
		@"Isaac",
		@"Isabella",
		@"Isaiah",
		@"Jace",
		@"Jack",
		@"Jackson",
		@"Jacob",
		@"James",
		@"Jasmine",
		@"Jason",
		@"Jaxon",
		@"Jayden",
		@"Jeremiah",
		@"Jocelyn",
		@"John",
		@"Jonathan",
		@"Jordan",
		@"Jose",
		@"Joseph",
		@"Joshua",
		@"Josiah",
		@"Juan",
		@"Julia",
		@"Julian",
		@"Justin",
		@"Katherine",
		@"Kayden",
		@"Kayla",
		@"Kaylee",
		@"Kennedy",
		@"Kevin",
		@"Khloe",
		@"Kimberly",
		@"Kylie",
		@"Landon",
		@"Lauren",
		@"Layla",
		@"Leah",
		@"Levi",
		@"Liam",
		@"Lillian",
		@"Lily",
		@"Logan",
		@"London",
		@"Lucas",
		@"Lucy",
		@"Luis",
		@"Luke",
		@"Lydia",
		@"Mackenzie",
		@"Madeline",
		@"Madelyn",
		@"Madison",
		@"Makayla",
		@"Mason",
		@"Matthew",
		@"Maya",
		@"Melanie",
		@"Mia",
		@"Michael",
		@"Molly",
		@"Morgan",
		@"Naomi",
		@"Natalie",
		@"Nathan",
		@"Nathaniel",
		@"Nevaeh",
		@"Nicholas",
		@"Noah",
		@"Nolan",
		@"Oliver",
		@"Olivia",
		@"Owen",
		@"Parker",
		@"Peyton",
		@"Piper",
		@"Reagan",
		@"Riley",
		@"Robert",
		@"Ryan",
		@"Ryder",
		@"Samantha",
		@"Samuel",
		@"Sarah",
		@"Savannah",
		@"Scarlett",
		@"Sebastian",
		@"Serenity",
		@"Skylar",
		@"Sofia",
		@"Sophia",
		@"Sophie",
		@"Stella",
		@"Sydney",
		@"Taylor",
		@"Thomas",
		@"Trinity",
		@"Tristan",
		@"Tyler",
		@"Victoria",
		@"Violet",
		@"William",
		@"Wyatt",
		@"Xavier",
		@"Zachary",
		@"Zoe",
		@"Zoey"
	];
}

//===============================================
#pragma mark -
#pragma mark View Methods
//===============================================

- (void)viewDidLoad {
	[super viewDidLoad];

	[self configureTableView:self.tableView];
}

//===============================================
#pragma mark -
#pragma mark Helper
//===============================================

- (void)configureTableView:(UITableView *)tableView {

	tableView.separatorInset = UIEdgeInsetsZero;

	[tableView registerClass:[UITableViewCell class] forCellReuseIdentifier:@"cellId"];

	UIView *tableFooterViewToGetRidOfBlankRows = [[UIView alloc] initWithFrame:CGRectZero];
	tableFooterViewToGetRidOfBlankRows.backgroundColor = [UIColor clearColor];
	tableView.tableFooterView = tableFooterViewToGetRidOfBlankRows;
}

//===============================================
#pragma mark -
#pragma mark UITableView
//===============================================

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {

	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {

	if (tableView == self.tableView) {
		return [self.names count];
	} else {
		return [self.searchResults count];
	}
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {

	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"cellId" forIndexPath:indexPath];

	NSString *name = (tableView == self.tableView) ? self.names[indexPath.row] : self.searchResults[indexPath.row];

	cell.textLabel.text = name;

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {

	[tableView deselectRowAtIndexPath:indexPath animated:YES];
}

//===============================================
#pragma mark -
#pragma mark UISearchDisplayDelegate
//===============================================

- (void)searchDisplayControllerWillBeginSearch:(UISearchDisplayController *)controller {
	NSLog(@"🔦 | will begin search");
}
- (void)searchDisplayControllerDidBeginSearch:(UISearchDisplayController *)controller {
	NSLog(@"🔦 | did begin search");
}
- (void)searchDisplayControllerWillEndSearch:(UISearchDisplayController *)controller {
	NSLog(@"🔦 | will end search");
}
- (void)searchDisplayControllerDidEndSearch:(UISearchDisplayController *)controller {
	NSLog(@"🔦 | did end search");
}
- (void)searchDisplayController:(UISearchDisplayController *)controller
  didLoadSearchResultsTableView:(UITableView *)tableView {
	NSLog(@"🔦 | did load table");
	[self configureTableView:tableView];
}
- (void)searchDisplayController:(UISearchDisplayController *)controller
	willUnloadSearchResultsTableView:(UITableView *)tableView {
	NSLog(@"🔦 | will unload table");
}
- (void)searchDisplayController:(UISearchDisplayController *)controller
 willShowSearchResultsTableView:(UITableView *)tableView {
	NSLog(@"🔦 | will show table");
}
- (void)searchDisplayController:(UISearchDisplayController *)controller
  didShowSearchResultsTableView:(UITableView *)tableView {
	NSLog(@"🔦 | did show table");
}
- (void)searchDisplayController:(UISearchDisplayController *)controller
 willHideSearchResultsTableView:(UITableView *)tableView {
	NSLog(@"🔦 | will hide table");
}
- (void)searchDisplayController:(UISearchDisplayController *)controller
  didHideSearchResultsTableView:(UITableView *)tableView {
	NSLog(@"🔦 | did hide table");
}
- (BOOL)searchDisplayController:(UISearchDisplayController *)controller
	shouldReloadTableForSearchString:(NSString *)searchString {
	NSLog(@"🔦 | should reload table for search string?");

	NSPredicate *predicate = [NSPredicate predicateWithFormat:@"SELF CONTAINS[cd] %@", searchString];
	self.searchResults = [self.names filteredArrayUsingPredicate:predicate];

	return YES;
}
- (BOOL)searchDisplayController:(UISearchDisplayController *)controller
shouldReloadTableForSearchScope:(NSInteger)searchOption {
	NSLog(@"🔦 | should reload table for search scope?");
	return YES;
}

@end
