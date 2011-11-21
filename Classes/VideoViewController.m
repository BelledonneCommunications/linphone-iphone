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

@synthesize mLandscape;
@synthesize mDisplayLand;
@synthesize mPreviewLand;
@synthesize mMuteLand;
@synthesize mHangUpLand;
@synthesize mCamSwitchLand;

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
	[mMute initWithOnImage:[UIImage imageNamed:@"mic_muted.png"]  offImage:[UIImage imageNamed:@"mic_active.png"] ];
	[mMuteLand initWithOnImage:[UIImage imageNamed:@"mic_muted.png"]  offImage:[UIImage imageNamed:@"mic_active.png"] ];
	[mCamSwitch setPreview:mPreview];
	[mCamSwitchLand setPreview:mPreviewLand];
	isFirst=TRUE;
}


-(void) configureOrientation:(UIInterfaceOrientation) oritentation  {
	int oldLinphoneOrientation = linphone_core_get_device_rotation([LinphoneManager getLc]);
	if (oritentation == UIInterfaceOrientationPortrait ) {
		[self.view addSubview:mPortrait];
		linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)mDisplay);	
		linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)mPreview);
		linphone_core_set_device_rotation([LinphoneManager getLc], 0);
		
	} else if (oritentation == UIInterfaceOrientationLandscapeRight ) {
		[self.view addSubview:mLandscape];
		linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)mDisplayLand);	
		linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)mPreviewLand);
		linphone_core_set_device_rotation([LinphoneManager getLc], 270);
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
    [super viewDidUnload];
	
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


-(void) viewDidDisappear:(BOOL)animated{
    [super viewDidDisappear:animated];
	[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	linphone_core_set_max_calls([LinphoneManager getLc], maxCall);
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
    [mMute reset];
    [mMuteLand reset];
	maxCall = linphone_core_get_max_calls([LinphoneManager getLc]);
	linphone_core_set_max_calls([LinphoneManager getLc], 1);
}

- (void) viewDidAppear:(BOOL)animated{
    [super viewDidAppear:animated];
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return  interfaceOrientation == UIInterfaceOrientationPortrait || interfaceOrientation == UIInterfaceOrientationLandscapeRight ;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self configureOrientation:self.interfaceOrientation];
	[mMute reset];
    [mMuteLand reset];
}
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
	[mLandscape removeFromSuperview];
	[mPortrait removeFromSuperview];
}
@end
