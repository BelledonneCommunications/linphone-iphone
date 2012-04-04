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

-(void) send{
    LinphoneCall *call=linphone_core_get_current_call([LinphoneManager getLc]);
    if (call && linphone_call_get_state(call)==LinphoneCallStreamsRunning) {
        linphone_core_send_dtmf([LinphoneManager getLc],'2');
    } else if (chatRoom) {
        linphone_chat_room_send_message(chatRoom, "2");
    }
}

-(void) onOn {
    [self send];
}

-(void) onOff {
    [self send];
}

-(bool) isInitialStateOn {
    return FALSE;
}

@end
