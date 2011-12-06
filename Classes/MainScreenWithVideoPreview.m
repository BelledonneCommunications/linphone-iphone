//
//  MainScreenWithVideoPreview.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 07/12/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "MainScreenWithVideoPreview.h"
#import <AVFoundation/AVFoundation.h>

@implementation MainScreenWithVideoPreview
@synthesize window;
@synthesize phoneMainView;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
}
*/

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
                        
    session = [[AVCaptureSession alloc] init];
    
   
    
    currentCamera = 0;
    

    
    AVCaptureVideoPreviewLayer* previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:session];
    
    previewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    previewLayer.frame = self.view.bounds;
    [self.view.layer addSublayer:previewLayer];
    
    [session beginConfiguration];
    [session setSessionPreset:AVCaptureSessionPresetHigh];
    [session commitConfiguration];
    
    [self useCameraAtIndex:0 startSession:NO];
}

-(void) switchCameraPressed {
    [self useCameraAtIndex: (currentCamera + 1) startSession:YES];
}

-(void) useCameraAtIndex:(NSInteger)camIndex startSession:(BOOL)start {
    [session stopRunning];
    
    if (input != nil)
        [session removeInput:input];
    
    NSError* error;
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc]init];
    NSArray* array = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    currentCamera = camIndex % [array count];
    AVCaptureDevice* device =  (AVCaptureDevice*) [array objectAtIndex:currentCamera];
    input = [[AVCaptureDeviceInput deviceInputWithDevice:device
                                                error:&error] retain];  
    
    [session addInput:input];
    
    [pool drain];
    
    if (start)
        [session startRunning];
}


-(void) showPreview:(BOOL) show {
    if (show && !session.running) {
        [window addSubview:self.view];
        [window sendSubviewToBack:self.view];
        [session startRunning];
    } else if (!show && session.running) {
        [self.view removeFromSuperview];
        [session stopRunning];
    }
}

-(void) viewDidAppear:(BOOL)animated {
    [phoneMainView.switchCamera addTarget:self action:@selector(switchCameraPressed) forControlEvents:UIControlEventTouchUpInside];
}

-(void) viewDidDisappear:(BOOL)animated {
    
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return NO;
}

@end
