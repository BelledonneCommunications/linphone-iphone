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
#import "oRTP/rtpsession.h"
#import "UITextField+DoneButton.h"
extern void libmsopenh264_init();

@interface ViewController () {

	MSWebCam* videoCam;
	MSWebCam* noCam;
	MSFilter* noWebCamFilter;
	MSFilter* videoCamFilter;

	RtpProfile*  profile;
	VideoStream* currentStream;
	IceSession*  iceSession;
	PayloadType* vp8_pt;
}
@end

@implementation ViewController
#define CAM_NAME "AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:1"

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
	[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(infoTimer:) userInfo:nil repeats:TRUE];
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

- (void)updateInfo {
	if( currentStream == NULL ){
		self.infoLabel.text = @"No stream";
		self.bandwidthLabel.text = @"";
		return;

	}
	const MSWebCam* currentCam = video_stream_get_camera(currentStream);
	if( currentCam )
		self.infoLabel.text = [NSString stringWithFormat:@"Stream running, current cam: %s", currentCam->name];
	else
		self.infoLabel.text = @"No Webcam ?!";

	MediaStream* ms = (MediaStream*)currentStream;
	float upload = rtp_session_get_rtp_send_bandwidth(ms->sessions.rtp_session);
	float download = rtp_session_get_rtp_recv_bandwidth(ms->sessions.rtp_session);

	self.bandwidthLabel.text = [NSString stringWithFormat:@"Upload: %f kbit/s, Download: %f kbit/s", upload/1000.0, download/1000.0];

}

#pragma mark - Timer callbacks
- (void)timer:(NSTimer*)timer {
	if( currentStream) {
		media_stream_iterate((MediaStream*)currentStream);
	}
}

- (void)infoTimer:(NSTimer*)timer {
	[self updateInfo];
}


#pragma mark - Actions

- (IBAction)onStartStreamsClick:(id)sender {
	if( currentStream ){
		VideoStream* stream = currentStream;
		currentStream = NULL;
		video_stream_stop_keep_source(stream);
		[self.startStreamLabel setTitle:@"Start streams" forState:UIControlStateNormal];

	} else {
		VideoStream * stream = video_stream_new(3456, 3457, FALSE);
		video_stream_set_native_window_id(stream, (unsigned long)self.remoteView);
		video_stream_set_native_preview_window_id(stream, (unsigned long)self.localView);
		video_stream_start_with_source(stream, profile, "127.0.0.1", 3456, "127.0.0.1", 3457, 103, 30, noCam, noWebCamFilter);
		currentStream = stream;
		[self.startStreamLabel setTitle:@"Stop streams" forState:UIControlStateNormal];
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
