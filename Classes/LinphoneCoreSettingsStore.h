//
//  LinphoneCoreSettingsStore.h
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 22/05/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "IASKSettingsStore.h"

#import "LinphoneManager.h"

@interface LinphoneCoreSettingsStore : IASKAbstractSettingsStore {
	NSDictionary *dict;
}

-(void) transformLinphoneCoreToKeys;

@end
