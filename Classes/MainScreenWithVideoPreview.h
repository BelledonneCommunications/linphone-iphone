//
//  MainScreenWithVideoPreview.h
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 07/12/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import "PhoneViewController.h"
 
@interface MainScreenWithVideoPreview : UIViewController {
    UIWindow *window;
    PhoneViewController* phoneMainView;
    
    
    AVCaptureSession* session;
    AVCaptureDeviceInput* input;
    
    int currentCamera;
}

-(void) showPreview:(BOOL) show;

-(void) useCameraAtIndex:(NSInteger)camIndex startSession:(BOOL)start;

@property (nonatomic, retain) IBOutlet PhoneViewController* phoneMainView;
@property (nonatomic, retain) IBOutlet UIWindow *window;
@end
