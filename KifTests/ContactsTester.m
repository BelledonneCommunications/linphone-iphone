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
    
    if ( phone ){
        [self setText:phone forContactNumbersIndex:0 inSection:ContactSections_Number];
    }
    
    if ( sip ){
        [self setText:sip forContactNumbersIndex:0 inSection:ContactSections_Sip];
    }
    
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    [tester tapViewWithAccessibilityLabel:@"Back"];
    
}

#pragma mark - Tests

- (void)testDeleteContact {
    NSString* contactName = [self getUUID];
    [self createContact:contactName lastName:@"dummy" phoneNumber:@"0102030405" SIPAddress:@"testios"];
    
    NSString* fullName = [contactName stringByAppendingString:@" dummy"];
    
    [tester tapViewWithAccessibilityLabel:fullName traits:UIAccessibilityTraitStaticText];
    
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    [tester scrollViewWithAccessibilityIdentifier:@"Contact numbers table" byFractionOfSizeHorizontal:0 vertical:-0.9];
    
    [tester tapViewWithAccessibilityLabel:@"Remove"];
    
    [tester waitForAbsenceOfViewWithAccessibilityLabel:@"Firstname, Lastname" value:fullName traits:UIAccessibilityTraitStaticText];
}

- (void)addNumbersToSection:(NSInteger)section numbers:(NSArray*)numbers {

    [tester tapViewWithAccessibilityLabel:@"Edit"];
    for(NSInteger i = 0; i<numbers.count;i++){
        [self setText:[numbers objectAtIndex:i] forContactNumbersIndex:i inSection:section];
        // now tap the "+" to add a new item. This is not optimal, since it will tap the first instance with that name.
        // we should do something like "[tester tapViewWithAccessibilityLabel:@"Insert linphone" atRowIndex:i inSection:section]"
        [tester tapViewWithAccessibilityLabel:@"Insert Linphone"];
    }
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    
    for(NSInteger i = 0; i<numbers.count;i++){
        [tester waitForViewWithAccessibilityLabel:[@"Linphone, " stringByAppendingString:[numbers objectAtIndex:i]]
                                           traits:UIAccessibilityTraitStaticText];
    }
}

- (void)testEditContact {
    NSString* contactName = [self getUUID];
    NSString* fullName = [contactName stringByAppendingString:@" dummy"];
    [self createContact:contactName lastName:@"dummy" phoneNumber:nil SIPAddress:nil];
    
    [tester tapViewWithAccessibilityLabel:fullName traits:UIAccessibilityTraitStaticText];
    
    /* Phone number */
    NSArray* phones = @[@"01234", @"56789"];
    NSLog(@"add phones");
    [self addNumbersToSection:ContactSections_Number numbers:phones];
    
    NSArray* SIPs = @[@"sip1", @"sip2"];
    [self addNumbersToSection:ContactSections_Sip numbers:SIPs];
    
    
    // remove all these numbers, doesn't quite work today...
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    for(NSInteger i = 0; i< (phones.count+SIPs.count); i++){
        [tester tapViewWithAccessibilityLabel:@"Delete Linphone"];
        [tester tapViewWithAccessibilityLabel:@"Delete"];
    }
    [tester tapViewWithAccessibilityLabel:@"Edit"];
    
    // then remove the contact
    [tester tapViewWithAccessibilityLabel:@"Edit"];

	[tester scrollViewWithAccessibilityIdentifier:@"Contact numbers table" byFractionOfSizeHorizontal:0 vertical:-0.9];

    [tester tapViewWithAccessibilityLabel:@"Remove"];
    
    [tester waitForAbsenceOfViewWithAccessibilityLabel:fullName traits:UIAccessibilityTraitStaticText];
    
}

@end
