//
//  ContactsTester.m
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 17/02/2015.
//
//

#import "ContactsTester.h"

#import "ContactDetailsTableViewController.h"

@implementation ContactsTester

#pragma mark - Setup

- (void)beforeEach {
	[super beforeEach];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Back"];
	}
	[tester tapViewWithAccessibilityLabel:@"Contacts"];
}

#pragma mark - Utils

- (void)setText:(NSString *)text forContactHeaderIndex:(NSInteger)idx {
	[tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:idx inSection:0]
		inTableViewWithAccessibilityIdentifier:@"Contact Name Table"];
	[tester enterTextIntoCurrentFirstResponder:text];
}

- (void)setText:(NSString *)text forContactNumbersIndex:(NSInteger)idx inSection:(NSInteger)section {
	[tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:idx inSection:section]
		inTableViewWithAccessibilityIdentifier:@"Contact numbers table"];
	[tester enterTextIntoCurrentFirstResponder:text];
}

- (void)createContact:(NSString *)firstName
			 lastName:(NSString *)lastName
		  phoneNumber:(NSString *)phone
		   SIPAddress:(NSString *)sip {

	XCTAssert(firstName != nil);
	[tester tapViewWithAccessibilityLabel:@"Add contact"];

	// check that the OK button is disabled
	[tester waitForViewWithAccessibilityLabel:@"Edit"
									   traits:UIAccessibilityTraitButton | UIAccessibilityTraitNotEnabled |
											  UIAccessibilityTraitSelected];

	[self setText:firstName forContactHeaderIndex:0];

	// entering text should enable the "edit" button
	[tester waitForViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton | UIAccessibilityTraitSelected];

	if (lastName)
		[self setText:lastName forContactHeaderIndex:1];

	if (phone) {
		[self setText:phone forContactNumbersIndex:0 inSection:ContactSections_Number];
	}

	if (sip) {
		[self setText:sip forContactNumbersIndex:0 inSection:ContactSections_Sip];
	}

	[tester tapViewWithAccessibilityLabel:@"Edit"];
}

#pragma mark - Tests

- (void)testCallContactWithInvalidPhoneNumber {
	NSString *contactName = [self getUUID];
	NSString *phone = @"+5 15 #0664;447*46";
	[self createContact:contactName lastName:@"dummy" phoneNumber:phone SIPAddress:nil];
	[tester tapViewWithAccessibilityLabel:[@"Linphone, " stringByAppendingString:phone]];
	[tester waitForViewWithAccessibilityLabel:[phone stringByAppendingString:@" not registered"]];
	[tester tapViewWithAccessibilityLabel:@"Dismiss"];
}

- (void)testDeleteContact {
	NSString *contactName = [self getUUID];
	[self createContact:contactName lastName:@"dummy" phoneNumber:@"0102030405" SIPAddress:[self me]];
	[tester tapViewWithAccessibilityLabel:@"Back"];

	NSString *fullName = [contactName stringByAppendingString:@" dummy"];

	[tester tapViewWithAccessibilityLabel:fullName traits:UIAccessibilityTraitStaticText];

	[tester tapViewWithAccessibilityLabel:@"Edit"];
	[tester scrollViewWithAccessibilityIdentifier:@"Contact numbers table" byFractionOfSizeHorizontal:0 vertical:-0.9];

	[tester tapViewWithAccessibilityLabel:@"Remove"];

	[tester waitForAbsenceOfViewWithAccessibilityLabel:@"Firstname, Lastname"
												 value:fullName
												traits:UIAccessibilityTraitStaticText];
}

- (void)tapCellForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section atX:(CGFloat)x {
	UITableView *tv = [self findTableView:@"Contact numbers table"];
	NSIndexPath *path = [NSIndexPath indexPathForRow:idx inSection:section];
	UITableViewCell *last =
		[tester waitForCellAtIndexPath:path inTableViewWithAccessibilityIdentifier:@"Contact numbers table"];
	XCTAssertNotNil(last);

	CGRect cellFrame = [last.contentView convertRect:last.contentView.frame toView:tv];
	[tv tapAtPoint:CGPointMake(x > 0 ? x : cellFrame.size.width + x, cellFrame.origin.y + cellFrame.size.height / 2.)];
	[tester waitForAnimationsToFinish];
}

- (void)tapRemoveButtonForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section {
	[self tapCellForRowAtIndexPath:idx inSection:section atX:-10];
}

- (void)tapEditButtonForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section {
	// tap the "+" to add a new item (or "-" to delete it).... WOW, this code is ugly!
	// the thing is: we don't handle the "+" button ourself (system stuff)
	// so it is not present in the tableview cell... so we tap on a fixed position of screen :)
	[self tapCellForRowAtIndexPath:idx inSection:section atX:10];
}

- (void)addEntries:(NSArray *)numbers inSection:(NSInteger)section {
	[tester tapViewWithAccessibilityLabel:@"Edit"];
	[self setText:[numbers objectAtIndex:0] forContactNumbersIndex:0 inSection:section];
	for (NSInteger i = 1; i < numbers.count; i++) {
		[self tapEditButtonForRowAtIndexPath:i - 1 inSection:section];
		[self setText:[numbers objectAtIndex:i] forContactNumbersIndex:i inSection:section];
	}
	[tester tapViewWithAccessibilityLabel:@"Edit"];

	for (NSInteger i = 0; i < numbers.count; i++) {
		[tester waitForViewWithAccessibilityLabel:[@"Linphone, " stringByAppendingString:[numbers objectAtIndex:i]]
										   traits:UIAccessibilityTraitStaticText];
	}
}

- (void)deleteContactEntryForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section {
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Delete" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Delete"];
	} else {
		// hack: Travis seems to be unable to click on delete for what ever reason
		[self tapRemoveButtonForRowAtIndexPath:idx inSection:section];
	}
}

- (void)testEditContact {
	NSString *contactName = [self getUUID];
	NSString *fullName = [contactName stringByAppendingString:@" dummy"];
	[self createContact:contactName lastName:@"dummy" phoneNumber:nil SIPAddress:nil];

	/* Phone number */
	NSArray *phones = @[ @"01234", @"56789" ];
	[self addEntries:phones inSection:ContactSections_Number];
	NSArray *SIPs = @[ @"sip1", @"sip2" ];
	[self addEntries:SIPs inSection:ContactSections_Sip];

	[tester tapViewWithAccessibilityLabel:@"Edit"];
	// remove all numbers
	for (NSInteger i = 0; i < phones.count; i++) {
		[self tapEditButtonForRowAtIndexPath:0 inSection:ContactSections_Number];
		[self deleteContactEntryForRowAtIndexPath:0 inSection:ContactSections_Number];
	}
	// remove all SIPs
	for (NSInteger i = 0; i < SIPs.count; i++) {
		[self tapEditButtonForRowAtIndexPath:0 inSection:ContactSections_Sip];
		[self deleteContactEntryForRowAtIndexPath:0 inSection:ContactSections_Sip];
	}
	[tester tapViewWithAccessibilityLabel:@"Edit"];

	// then remove the contact
	[tester tapViewWithAccessibilityLabel:@"Edit"];

	[tester scrollViewWithAccessibilityIdentifier:@"Contact numbers table" byFractionOfSizeHorizontal:0 vertical:-0.9];

	[tester tapViewWithAccessibilityLabel:@"Remove"];

	[tester waitForAbsenceOfViewWithAccessibilityLabel:fullName traits:UIAccessibilityTraitStaticText];
}

@end
