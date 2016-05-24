//
//  ContactsTester.m
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 17/02/2015.
//
//

#import "ContactsTester.h"

#import "ContactDetailsTableView.h"
#import "UIContactCell.h"

@implementation ContactsTester

#pragma mark - Setup

- (void)beforeAll {
	[self switchToValidAccountIfNeeded];
}

- (void)beforeEach {
	[super beforeEach];
	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Back" error:nil]) {
		[tester tapViewWithAccessibilityLabel:@"Back"];
	}
	[tester tapViewWithAccessibilityLabel:@"Contacts"];
}

#pragma mark - Utils

- (void)tapCellForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section atX:(CGFloat)x {
	UITableView *tv = [self findTableView:@"Contact table"];
	NSIndexPath *path = [NSIndexPath indexPathForRow:idx inSection:section];
	UITableViewCell *cell =
		[tester waitForCellAtIndexPath:path inTableViewWithAccessibilityIdentifier:@"Contact table"];
	XCTAssertNotNil(cell);

	CGRect cellFrame = [cell.contentView convertRect:cell.contentView.frame toView:tv];
	[tv tapAtPoint:CGPointMake(x > 0 ? x : tv.superview.frame.size.width + x,
							   cellFrame.origin.y + cellFrame.size.height / 2.)];
	[tester waitForAnimationsToFinish];
}

- (void)tapRemoveButtonForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section {
	[self tapCellForRowAtIndexPath:idx inSection:section atX:-7];
}

- (void)setText:(NSString *)text forIndex:(NSInteger)idx inSection:(NSInteger)section {
	[tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:idx inSection:section]
		inTableViewWithAccessibilityIdentifier:@"Contact table"];
	[tester clearTextFromAndThenEnterTextIntoCurrentFirstResponder:text];
}

- (void)addEntries:(NSArray *)numbers inSection:(NSInteger)section {
	[tester tapViewWithAccessibilityLabel:@"Edit"];
	NSString *name = (section == ContactSections_Sip) ? @"Add new SIP address" : @"Add new phone number";
	[self setText:[numbers objectAtIndex:0] forIndex:0 inSection:section];
	for (NSInteger i = 1; i < numbers.count; i++) {
		[tester tapViewWithAccessibilityLabel:name traits:UIAccessibilityTraitButton];
		[self setText:[numbers objectAtIndex:i] forIndex:i inSection:section];
	}
	[tester tapViewWithAccessibilityLabel:@"Edit"];

	for (NSInteger i = 0; i < numbers.count; i++) {
		[tester waitForViewWithAccessibilityLabel:[@"Call " stringByAppendingString:[numbers objectAtIndex:i]]
										   traits:UIAccessibilityTraitButton];
	}
}

- (void)deleteContactEntryForRowAtIndexPath:(NSInteger)idx inSection:(NSInteger)section {
	//	if ([tester tryFindingTappableViewWithAccessibilityLabel:@"Delete" error:nil]) {
	//		[tester tapViewWithAccessibilityLabel:@"Delete"];
	//	} else {
	// hack: Travis seems to be unable to click on delete for what ever reason
		[self tapRemoveButtonForRowAtIndexPath:idx inSection:section];
		//	}
}

#pragma mark - Tests

- (void)testCallContactWithInvalidPhoneNumber {
	NSString *contactName = [self getUUID];
	NSString *phone = @"+5 15 #0664;447*46";
	[self createContact:contactName lastName:@"dummy" phoneNumber:phone SIPAddress:nil];
	[tester tapViewWithAccessibilityLabel:[@"Call " stringByAppendingString:phone]];
	[tester waitForViewWithAccessibilityLabel:[phone stringByAppendingString:@" is not registered."]];
	[tester tapViewWithAccessibilityLabel:@"Cancel"];
}

- (void)testDeleteContact {
	NSString *contactName = [self getUUID];
	[self createContact:contactName lastName:@"dummy" phoneNumber:@"0102030405" SIPAddress:[self me]];
	[tester tapViewWithAccessibilityLabel:@"Back"];

	NSString *fullName = [contactName stringByAppendingString:@" dummy"];

	[tester tapViewWithAccessibilityLabel:fullName traits:UIAccessibilityTraitStaticText];

	[tester tapViewWithAccessibilityLabel:@"Edit"];

	[tester tapViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton];
	[tester tapViewWithAccessibilityLabel:@"DELETE" traits:UIAccessibilityTraitButton];
}

- (void)testDeleteContactWithSwipe {
	NSString *contactName = [self getUUID];
	[self createContact:contactName lastName:@"dummy" phoneNumber:@"123" SIPAddress:@"ola"];
	[tester tapViewWithAccessibilityLabel:@"Back"];
	NSString *fullName = [contactName stringByAppendingString:@" dummy"];

	[tester swipeViewWithAccessibilityLabel:fullName inDirection:KIFSwipeDirectionLeft];
	[tester tapViewWithAccessibilityLabel:@"Delete"];

	// we should not find this contact anymore
	XCTAssert([tester tryFindingViewWithAccessibilityLabel:fullName error:nil] == NO);
}

- (void)testEditContact {
	NSString *contactName = [self getUUID];
	[self createContact:contactName lastName:@"dummy" phoneNumber:@"111" SIPAddress:nil];

	/* Phone number */
	NSArray *phones = @[ @"01234", @"56789" ];
	[self addEntries:phones inSection:ContactSections_Number];
	NSArray *SIPs = @[ @"sip1", @"sip2" ];
	[self addEntries:SIPs inSection:ContactSections_Sip];

	[tester tapViewWithAccessibilityLabel:@"Edit"];
	// remove all numbers
	for (NSInteger i = 0; i < phones.count; i++) {
		[self deleteContactEntryForRowAtIndexPath:0 inSection:ContactSections_Number];
	}
	// remove all SIPs
	for (NSInteger i = 0; i < SIPs.count; i++) {
		[self deleteContactEntryForRowAtIndexPath:0 inSection:ContactSections_Sip];
	}
	[tester tapViewWithAccessibilityLabel:@"Edit"];

	// then remove the contact
	[tester tapViewWithAccessibilityLabel:@"Edit"];

	[tester scrollViewWithAccessibilityIdentifier:@"Contact table" byFractionOfSizeHorizontal:0 vertical:-0.9];

	[tester tapViewWithAccessibilityLabel:@"Delete" traits:UIAccessibilityTraitButton];
	[tester tapViewWithAccessibilityLabel:@"DELETE" traits:UIAccessibilityTraitButton];
}

@end
