/* VideoViewController.m
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

#import "VideoViewController.h"
#import "LinphoneManager.h"
#import <AudioToolbox/AudioToolbox.h>

@implementation VideoViewController
@synthesize mPortrait;
@synthesize mDisplay;
@synthesize mPreview;
@synthesize mMute;
@synthesize mHangUp;
@synthesize mCamSwitch;
@synthesize mCallQuality;

@synthesize mLandscapeRight;
@synthesize mDisplayLandRight;
@synthesize mPreviewLandRight;
@synthesize mMuteLandRight;
@synthesize mHangUpLandRight;
@synthesize mCamSwitchLandRight;
@synthesize mCallQualityLandRight;

@synthesize mLandscapeLeft;
@synthesize mDisplayLandLeft;
@synthesize mPreviewLandLeft;
@synthesize mMuteLandLeft;
@synthesize mHangUpLandLeft;
@synthesize mCamSwitchLandLeft;
@synthesize mCallQualityLandLeft;

NSTimer *callQualityRefresher;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)dealloc
{
	[mCallQuality release];
	[mCallQualityLandRight release];
	[mCallQualityLandLeft release];
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
	/*[mMute initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] debugName:"MUTE button"];
	[mMuteLandRight initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] debugName:"MUTE2 button"];
	[mMuteLandLeft initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] debugName:"MUTE3 button"];*/
	[mCamSwitch setPreview:mPreview];
	[mCamSwitchLandRight setPreview:mPreviewLandRight];
	[mCamSwitchLandLeft setPreview:mPreviewLandLeft];
	
	isFirst=TRUE;
}

- (void) updateCallQualityIndicator
{
	LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (!call)
        return;
    
	if (linphone_call_get_average_quality(call) >= 4) {
		[mCallQuality setImage: [UIImage imageNamed:@"stat_sys_signal_4.png"]];
		[mCallQualityLandRight setImage: [UIImage imageNamed:@"stat_sys_signal_4.png"]];
		[mCallQualityLandLeft setImage: [UIImage imageNamed:@"stat_sys_signal_4.png"]];
	}
	else if (linphone_call_get_average_quality(call) >= 3) {
		[mCallQuality setImage: [UIImage imageNamed:@"stat_sys_signal_3.png"]];
		[mCallQualityLandRight setImage: [UIImage imageNamed:@"stat_sys_signal_3.png"]];
		[mCallQualityLandLeft setImage: [UIImage imageNamed:@"stat_sys_signal_3.png"]];
	}
	else if (linphone_call_get_average_quality(call) >= 2) {
		[mCallQuality setImage: [UIImage imageNamed:@"stat_sys_signal_2.png"]];
		[mCallQualityLandRight setImage: [UIImage imageNamed:@"stat_sys_signal_2.png"]];
		[mCallQualityLandLeft setImage: [UIImage imageNamed:@"stat_sys_signal_2.png"]];
	}
	else if (linphone_call_get_average_quality(call) >= 1) {
		[mCallQuality setImage: [UIImage imageNamed:@"stat_sys_signal_1.png"]];
		[mCallQualityLandRight setImage: [UIImage imageNamed:@"stat_sys_signal_1.png"]];
		[mCallQualityLandLeft setImage: [UIImage imageNamed:@"stat_sys_signal_1.png"]];
	}
	else {
		[mCallQuality setImage: [UIImage imageNamed:@"stat_sys_signal_0.png"]];
		[mCallQualityLandRight setImage: [UIImage imageNamed:@"stat_sys_signal_0.png"]];
		[mCallQualityLandLeft setImage: [UIImage imageNamed:@"stat_sys_signal_0.png"]];
	}
}


-(void) configureOrientation:(UIInterfaceOrientation) oritentation  {
	int oldLinphoneOrientation = linphone_core_get_device_rotation([LinphoneManager getLc]);
	if (oritentation == UIInterfaceOrientationPortrait ) {
		[self.view addSubview:mPortrait];
		linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)mDisplay);	
		linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)mPreview);
		linphone_core_set_device_rotation([LinphoneManager getLc], 0);

	} else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
		[self.view addSubview:mLandscapeRight];
		linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)mDisplayLandRight);	
		linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)mPreviewLandRight);
		linphone_core_set_device_rotation([LinphoneManager getLc], 270);

	} else if (oritentation == UIInterfaceOrientationLandscapeLeft ) {
		[self.view addSubview:mLandscapeLeft];
		linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)mDisplayLandLeft);	
		linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)mPreviewLandLeft);
		linphone_core_set_device_rotation([LinphoneManager getLc], 90);
	}	
	if ((oldLinphoneOrientation != linphone_core_get_device_rotation([LinphoneManager getLc]))
		&& linphone_core_get_current_call([LinphoneManager getLc])) {
		//Orientation has change, must call update call
		linphone_core_update_call([LinphoneManager getLc], linphone_core_get_current_call([LinphoneManager getLc]), NULL);
	}
}

-(void) configureOrientation {
    [self configureOrientation:self.interfaceOrientation]; 
}

- (void)viewDidUnload
{
	[mCallQuality release];
	mCallQuality = nil;
	[self setMCallQualityLandRight:nil];
	[self setMCallQualityLandLeft:nil];
    [super viewDidUnload];
	
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


-(void) viewDidDisappear:(BOOL)animated{
    [super viewDidDisappear:animated];
	[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	linphone_core_set_max_calls([LinphoneManager getLc], maxCall);
	
	if (callQualityRefresher != nil) {
        [callQualityRefresher invalidate];
        callQualityRefresher=nil;
	}
}

-(void) viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
    //redirect audio to speaker
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
							 , sizeof (audioRouteOverride)
							 , &audioRouteOverride);
    
    
    [self performSelectorOnMainThread:@selector(configureOrientation)
             						   withObject:nil 
             						waitUntilDone:YES];
    [mMute update];
    [mMuteLandRight update];
	[mMuteLandLeft update];
	maxCall = linphone_core_get_max_calls([LinphoneManager getLc]);
	linphone_core_set_max_calls([LinphoneManager getLc], 1);
}

- (void) viewDidAppear:(BOOL)animated{
    [super viewDidAppear:animated];
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	
	callQualityRefresher = [NSTimer scheduledTimerWithTimeInterval:1
															target:self 
														  selector:@selector(updateCallQualityIndicator) 
														  userInfo:nil 
														   repeats:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    BOOL result = interfaceOrientation == UIInterfaceOrientationPortrait 
			|| interfaceOrientation == UIInterfaceOrientationLandscapeRight 
			|| interfaceOrientation == UIInterfaceOrientationLandscapeLeft;
    
    return result;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self configureOrientation:self.interfaceOrientation];
	[mMute update];
    [mMuteLandRight update];
	[mMuteLandLeft update];
}
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
	[mLandscapeLeft removeFromSuperview];
	[mLandscapeRight removeFromSuperview];
	[mPortrait removeFromSuperview];
}
@end
