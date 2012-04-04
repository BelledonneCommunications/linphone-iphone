//
//  UILightButton.h
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 31/01/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "UIToggleButton.h"
#include "linphonecore.h"


@interface UILightButton : UIToggleButton<UIToggleButtonDelegate> {
@public
    LinphoneChatRoom* chatRoom;
    
}

-(void) send;

@end

