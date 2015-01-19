//
//  LinphoneTestCase.m
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 19/01/2015.
//
//

#import "LinphoneTestCase.h"

@implementation LinphoneTestCase

- (void)beforeAll{
    [tester acknowledgeSystemAlert];
    [super beforeAll];
}

@end
