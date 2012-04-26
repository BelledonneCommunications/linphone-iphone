//
//  VideoZoomHandler.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 25/04/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "VideoZoomHandler.h"
#include "linphonecore.h"
#import "LinphoneManager.h"

@implementation VideoZoomHandler


-(void) zoomInOut:(UITapGestureRecognizer*) reco {
    if (zoomLevel != 1)
        zoomLevel = 1;
    else
        zoomLevel = 2;
    
    if (zoomLevel != 1) {
        CGPoint point = [reco locationInView:videoView];
        cx = point.x / videoView.frame.size.width;
        cy = 1 - point.y / videoView.frame.size.height;
    } else {
        cx = cy = 0.5;
    }
    linphone_call_zoom_video(linphone_core_get_current_call([LinphoneManager getLc]), zoomLevel, &cx, &cy);
}

-(void) videoPan:(UIPanGestureRecognizer*) reco {
    if (zoomLevel <= 1.0)
        return;
    
    float x,y;
    CGPoint translation = [reco translationInView:videoView];
    if ([reco state] == UIGestureRecognizerStateEnded) {
        cx -= translation.x / videoView.frame.size.width;
        cy += translation.y / videoView.frame.size.height; 
        x = cx;
        y = cy;
    } else if ([reco state] == UIGestureRecognizerStateChanged) {
        x = cx - translation.x / videoView.frame.size.width;
        y = cy + translation.y / videoView.frame.size.height; 
        [reco setTranslation:CGPointMake(0, 0) inView:videoView];
    } else {
        return;
    }
    
    linphone_call_zoom_video(linphone_core_get_current_call([LinphoneManager getLc]), zoomLevel, &x, &y);
    cx = x;
    cy = y;
}

-(void) pinch:(UIPinchGestureRecognizer*) reco {
    float s = zoomLevel;
    // CGPoint point = [reco locationInView:videoGroup];
    // float ccx = cx + (point.x / videoGroup.frame.size.width - 0.5) / s;
    // float ccy = cy - (point.y / videoGroup.frame.size.height - 0.5) / s;
    if ([reco state] == UIGestureRecognizerStateEnded) {
        zoomLevel = MAX(MIN(zoomLevel * reco.scale, 3.0), 1.0);
        s = zoomLevel;
        // cx = ccx;
        // cy = ccy;
    } else if ([reco state] == UIGestureRecognizerStateChanged) {
        s = zoomLevel * reco.scale;
        s = MAX(MIN(s, 3.0), 1.0);
    } else if ([reco state] == UIGestureRecognizerStateBegan) {
        
    } else {
        return;
    }
    
    
    linphone_call_zoom_video(linphone_core_get_current_call([LinphoneManager getLc]), s, &cx, &cy);
}

-(void) resetZoom {
    zoomLevel = 1;
    cx = cy = 0.5;
}


-(void) setup: (UIView*) view {
    videoView = view;
    
    UITapGestureRecognizer* doubleFingerTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(zoomInOut:)];
    [doubleFingerTap setNumberOfTapsRequired:2];
    [doubleFingerTap setNumberOfTouchesRequired:1];
    [videoView addGestureRecognizer:doubleFingerTap];

    UIPanGestureRecognizer* pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(videoPan:)];
    [videoView addGestureRecognizer:pan];
    UIPinchGestureRecognizer* pinchReco = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(pinch:)];
    [videoView addGestureRecognizer:pinchReco];    

    [doubleFingerTap release];
    [pan release];
    [pinchReco release];
    
    [self resetZoom];
}
@end
