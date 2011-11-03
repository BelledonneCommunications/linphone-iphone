//
//  CallDelegate.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 03/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "CallDelegate.h"

@implementation CallDelegate

@synthesize call;
@synthesize delegate;

-(void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    [delegate actionSheet:actionSheet clickedButtonAtIndex:buttonIndex withUserDatas:call];
}

@end
