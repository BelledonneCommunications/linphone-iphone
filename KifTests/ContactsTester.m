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

- (void)beforeAll {
    [tester tapViewWithAccessibilityLabel:@"Contacts"];
}

#pragma mark - Utils

- (void)setText:(NSString*)text forContactHeaderIndex:(NSInteger)idx {
    [tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:idx inSection:0] inTableViewWithAccessibilityIdentifier:@"Contact Name Table"];
    [tester enterTextIntoCurrentFirstResponder:text];
}

- (void)setText:(NSString*)text forContactNumbersIndex:(NSInteger)idx inSection:(NSInteger)section {
    [tester tapRowAtIndexPath:[NSIndexPath indexPathForRow:idx inSection:section] inTableViewWithAccessibilityIdentifier:@"Contact numbers table"];
    [tester enterTextIntoCurrentFirstResponder:text];
}

- (void)createContact:(NSString*)firstName lastName:(NSString*)lastName phoneNumber:(NSString*)phone SIPAddress:(NSString*)sip {
    
    XCTAssert(firstName != nil);
    [tester tapViewWithAccessibilityLabel:@"Add contact"];
    
    // check that the OK button is disabled
    [tester waitForViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton|UIAccessibilityTraitNotEnabled|UIAccessibilityTraitSelected];
    
    [self setText:firstName forContactHeaderIndex:0];
    
    // entering text should enable the "edit" button
    [tester waitForViewWithAccessibilityLabel:@"Edit" traits:UIAccessibilityTraitButton|UIAccessibilityTraitSelected];
    
    if( lastName )
        [self setText:lastName forContactHeaderIndex:1];
    
    if ( phone )
    [self setText:phone forContactNumbersIndex:0 inSection:ContactSections_Number];
    
    if (sip)
        [self setText:sip forContactNumbersIndex:0 inSection:ContactSections_Sip];
    
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    [tester tapViewWithAccessibilityLabel:@"Back"];
    
}

#pragma mark - Tests

- (void)testDeleteContact {
    NSString* contactName = [self getUUID];
    [self createContact:contactName lastName:@"dummy" phoneNumber:@"0102030405" SIPAddress:@"testios"];
    
    NSString* fullName = [contactName stringByAppendingString:@" dummy"];
    
    [tester tapViewWithAccessibilityLabel:@"Firstname, Lastname" value:fullName traits:UIAccessibilityTraitStaticText];
    
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    [tester scrollViewWithAccessibilityIdentifier:@"Contact numbers table" byFractionOfSizeHorizontal:0 vertical:-0.9];
    
    [tester tapViewWithAccessibilityLabel:@"Remove"];
    
    [tester waitForAbsenceOfViewWithAccessibilityLabel:@"Firstname, Lastname" value:fullName traits:UIAccessibilityTraitStaticText];
}

@end
