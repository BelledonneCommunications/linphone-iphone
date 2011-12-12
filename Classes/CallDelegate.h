//
//  CallDelegate.h
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 03/11/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "linphonecore.h"


@protocol UIActionSheetCustomDelegate 
- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex withUserDatas:(void*) datas;
@end


@interface CallDelegate : NSObject<UIActionSheetDelegate> {

    LinphoneCall* call;
    id<UIActionSheetCustomDelegate> delegate;
}

@property (nonatomic) LinphoneCall* call;
@property (nonatomic, retain) id delegate;

@end
