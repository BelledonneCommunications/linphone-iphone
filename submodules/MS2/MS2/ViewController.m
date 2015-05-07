//
//  ViewController.m
//  MS2
//
//  Created by guillaume on 06/05/2015.
//  Copyright (c) 2015 Belldonne Communications. All rights reserved.
//

#import "ViewController.h"
#include <ifaddrs.h>
#include <arpa/inet.h>

#import "mediastreamer2/mediastream.h"
#import "UITextField+DoneButton.h"
extern void libmsopenh264_init();

@interface ViewController () {

	MSWebCam* videoCam;
	MSWebCam* noCam;
	MSFilter* noWebCamFilter;
	MSFilter* videoCamFilter;

	RtpProfile* profile;
	VideoStream* currentStream;
	IceSession* iceSession;
	PayloadType* vp8_pt;
}
@end

@implementation ViewController
#define CAM_NAME "AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:0" /*"AV Capture: Back Camera"*/

- (void)viewDidLoad {
	[super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	ortp_init();
	ortp_set_log_level_mask(ORTP_DEBUG|ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	ms_init();
	libmsopenh264_init(); /*no plugin on IOS/Android */

	NSString* nowebFile = @"nowebcamCIF.jpg";
	NSString* path = [[NSBundle mainBundle] pathForResource:[nowebFile stringByDeletingPathExtension]
													 ofType:[nowebFile pathExtension]];
	ms_static_image_set_default_image([path UTF8String]);


	noCam = ms_web_cam_manager_get_cam(ms_web_cam_manager_get(), "StaticImage: Static picture");
	videoCam = ms_web_cam_manager_get_cam(ms_web_cam_manager_get(), CAM_NAME);

	noWebCamFilter = ms_web_cam_create_reader(noCam);

	// open the cam immediately
	videoCamFilter = ms_web_cam_create_reader(videoCam);

	currentStream = NULL;
	profile = rtp_profile_clone_full(&av_profile);
	rtp_profile_set_payload(profile,103,&payload_type_vp8);

	vp8_pt = payload_type_clone(rtp_profile_get_payload(profile, 103));

	// Enable AVPF
	PayloadTypeAvpfParams avpf_params;
	payload_type_set_flag(vp8_pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
	avpf_params.features = PAYLOAD_TYPE_AVPF_FIR | PAYLOAD_TYPE_AVPF_PLI | PAYLOAD_TYPE_AVPF_SLI | PAYLOAD_TYPE_AVPF_RPSI;
	avpf_params.trr_interval = 3000;
	payload_type_set_avpf_params(vp8_pt, avpf_params);

	vp8_pt->normal_bitrate = 500000;

	[NSTimer scheduledTimerWithTimeInterval:0.02 target:self selector:@selector(timer:) userInfo:nil repeats:TRUE];
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

- (void)timer:(NSTimer*)timer {
	if( currentStream) media_stream_iterate((MediaStream*)currentStream);
}


- (NSString *)getIPAddress {

	NSString *address = @"error";
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0) {
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL) {
			if(temp_addr->ifa_addr->sa_family == AF_INET) {
				// Check if interface is en0 which is the wifi connection on the iPhone
				NSString* ifa = [NSString stringWithUTF8String:temp_addr->ifa_name];
				if([ifa isEqualToString:@"en0"] || [ifa isEqualToString:@"en1"]) {
					// Get NSString from C String
					address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];
					break;

				}

			}

			temp_addr = temp_addr->ifa_next;
		}
	}
	// Free memory
	freeifaddrs(interfaces);
	return address;

}

- (void)updateInfo {
	if( currentStream == NULL )return;
	const MSWebCam* currentCam = video_stream_get_camera(currentStream);
	if( currentCam )
		self.infoLabel.text = [NSString stringWithFormat:@"Stream running, current cam: %s", currentCam->name];
	else
		self.infoLabel.text = @"No Webcam ?!";

}

- (IBAction)onStartStreamsClick:(id)sender {
	if( currentStream ){
		VideoStream* stream = currentStream;
		currentStream = NULL;
		video_stream_stop(stream);

	} else {
		VideoStream * stream = video_stream_new(3456, 3457, FALSE);
		video_stream_set_native_window_id(stream, (unsigned long)self.remoteView);
		video_stream_set_native_preview_window_id(stream, (unsigned long)self.localView);
		video_stream_start_with_source(stream, profile, "127.0.0.1", 3456, "127.0.0.1", 3457, 103, 30, noCam, noWebCamFilter);
		currentStream = stream;
		[self updateInfo];
	}

}

- (IBAction)changeCamUp:(id)sender {
	if(currentStream == NULL )return;

	NSLog(@"Restore static cam (%p)", noWebCamFilter);
	video_stream_change_source_filter(currentStream, noCam, noWebCamFilter, TRUE);

	[self updateInfo];

}

- (IBAction)changeCamDown:(id)sender {
	if (currentStream == NULL) {
		return;
	}
	NSLog(@"Use real camera (%p)", videoCamFilter);
	video_stream_change_source_filter(currentStream, videoCam, videoCamFilter, TRUE);
	[self updateInfo];
}


@end
