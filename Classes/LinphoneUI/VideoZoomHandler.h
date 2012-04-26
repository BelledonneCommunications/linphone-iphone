//
//  VideoZoomHandler.h
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 25/04/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface VideoZoomHandler : NSObject {
    float zoomLevel, cx, cy;
    UIView* videoView;
}
-(void) setup: (UIView*) videoView;
-(void) resetZoom;

@end
