/* MainScreenWithVideoPreview.h
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */     
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

-(void) initVideoPreview {
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

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
                        
    bool enableVideo = [[NSUserDefaults standardUserDefaults] boolForKey:@"enable_video_preference"];
    
    if (enableVideo) {
        [self initVideoPreview ];
    }
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
    bool enableVideo = [[NSUserDefaults standardUserDefaults] boolForKey:@"enable_video_preference"];
    
    if (enableVideo) {
        if (session == nil) {
            [self initVideoPreview];
        }
        
        if (show && !session.running) {
            [window addSubview:self.view];
            [window sendSubviewToBack:self.view];
            [session startRunning];
        } else if (!show && session.running) {
            [self.view removeFromSuperview];
            [session stopRunning];
        }
    } else {
        if (session != nil)
            [session stopRunning];
        [self.view removeFromSuperview];
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
