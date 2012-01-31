//
//  UILightButton.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 31/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "UILightButton.h"
#import "LinphoneManager.h"

@implementation UILightButton

-(void) onOn {
    linphone_core_send_dtmf([LinphoneManager getLc],'2');
}

-(void) onOff {
    linphone_core_send_dtmf([LinphoneManager getLc],'2');
}

-(bool) isInitialStateOn {
    return FALSE;
}

@end
