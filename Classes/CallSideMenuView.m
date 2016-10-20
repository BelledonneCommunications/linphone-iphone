//
//  SideMenuViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import "CallSideMenuView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation CallSideMenuView {
	NSTimer *updateTimer;
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	if (updateTimer != nil) {
		[updateTimer invalidate];
	}
	updateTimer = [NSTimer scheduledTimerWithTimeInterval:1
												   target:self
												 selector:@selector(updateStats:)
												 userInfo:nil
												  repeats:YES];

	[self updateStats:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	if (updateTimer != nil) {
		[updateTimer invalidate];
		updateTimer = nil;
	}
}

- (IBAction)onLateralSwipe:(id)sender {
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

+ (NSString *)iceToString:(LinphoneIceState)state {
	switch (state) {
		case LinphoneIceStateNotActivated:
			return NSLocalizedString(@"Not activated", @"ICE has not been activated for this call");
			break;
		case LinphoneIceStateFailed:
			return NSLocalizedString(@"Failed", @"ICE processing has failed");
			break;
		case LinphoneIceStateInProgress:
			return NSLocalizedString(@"In progress", @"ICE process is in progress");
			break;
		case LinphoneIceStateHostConnection:
			return NSLocalizedString(@"Direct connection",
									 @"ICE has established a direct connection to the remote host");
			break;
		case LinphoneIceStateReflexiveConnection:
			return NSLocalizedString(
				@"NAT(s) connection",
				@"ICE has established a connection to the remote host through one or several NATs");
			break;
		case LinphoneIceStateRelayConnection:
			return NSLocalizedString(@"Relay connection", @"ICE has established a connection through a relay");
			break;
	}
}

+ (NSString*)afinetToString:(int)remote_family {
	return (remote_family == LinphoneAddressFamilyUnspec) ? @"Unspecified":(remote_family == LinphoneAddressFamilyInet) ? @"IPv4" : @"IPv6";
}

+ (NSString *)mediaEncryptionToString:(LinphoneMediaEncryption)enc {
	switch (enc) {
		case LinphoneMediaEncryptionDTLS:
			return @"DTLS";
		case LinphoneMediaEncryptionSRTP:
			return @"SRTP";
		case LinphoneMediaEncryptionZRTP:
			return @"ZRTP";
		case LinphoneMediaEncryptionNone:
			break;
	}
	return NSLocalizedString(@"None", nil);
}

- (NSString *)updateStatsForCall:(LinphoneCall *)call stream:(LinphoneStreamType)stream {
	NSMutableString *result = [[NSMutableString alloc] init];
	const PayloadType *payload = NULL;
	const LinphoneCallStats *stats;
	const LinphoneCallParams *params = linphone_call_get_current_params(call);
	NSString *name;

	switch (stream) {
		case LinphoneStreamTypeAudio:
			name = @"Audio";
			payload = linphone_call_params_get_used_audio_codec(params);
			stats = linphone_call_get_audio_stats(call);
			break;
		case LinphoneStreamTypeText:
			name = @"Text";
			payload = linphone_call_params_get_used_text_codec(params);
			stats = linphone_call_get_text_stats(call);
			break;
		case LinphoneStreamTypeVideo:
			name = @"Video";
			payload = linphone_call_params_get_used_video_codec(params);
			stats = linphone_call_get_video_stats(call);
			break;
		case LinphoneStreamTypeUnknown:
			break;
	}
	if (payload == NULL) {
		return result;
	}

	[result appendString:@"\n"];
	[result appendString:name];
	[result appendString:@"\n"];

	[result appendString:[NSString stringWithFormat:@"Codec: %s/%iHz", payload->mime_type, payload->clock_rate]];
	if (stream == LinphoneStreamTypeAudio) {
		[result appendString:[NSString stringWithFormat:@"/%i channels", payload->channels]];
	}
	[result appendString:@"\n"];
	// Encoder & decoder descriptions
	const char *enc_desc = ms_factory_get_encoder(linphone_core_get_ms_factory(LC), payload->mime_type)->text;
	const char *dec_desc = ms_factory_get_decoder(linphone_core_get_ms_factory(LC), payload->mime_type)->text;
	if (strcmp(enc_desc, dec_desc) == 0) {
		[result appendString:[NSString stringWithFormat:@"Encoder/Decoder: %s", enc_desc]];
		[result appendString:@"\n"];
	} else {
		[result appendString:[NSString stringWithFormat:@"Encoder: %s", enc_desc]];
		[result appendString:@"\n"];
		[result appendString:[NSString stringWithFormat:@"Decoder: %s", dec_desc]];
		[result appendString:@"\n"];
	}

	if (stats != NULL) {
		[result
			appendString:[NSString stringWithFormat:@"Download bandwidth: %1.1f kbits/s", stats->download_bandwidth]];
		[result appendString:@"\n"];
		[result appendString:[NSString stringWithFormat:@"Upload bandwidth: %1.1f kbits/s", stats->upload_bandwidth]];
		[result appendString:@"\n"];
		[result appendString:[NSString stringWithFormat:@"ICE state: %@", [self.class iceToString:stats->ice_state]]];
		[result appendString:@"\n"];
		
		[result appendString:[NSString stringWithFormat:@"Afinet: %@", [self.class afinetToString:stats->rtp_remote_family]]];
		[result appendString:@"\n"];

		// RTP stats section (packet loss count, etc)
		rtp_stats_t rtp_stats = linphone_call_stats_get_rtp_stats(stats);
		[result
			appendString:[NSString stringWithFormat:
									   @"RTP packets: %llu total, %lld cum loss, %llu discarded, %llu OOT, %llu bad",
									   rtp_stats.packet_recv, rtp_stats.cum_packet_loss, rtp_stats.discarded,
									   rtp_stats.outoftime, rtp_stats.bad]];
		[result appendString:@"\n"];
		[result appendString:[NSString stringWithFormat:@"Sender loss rate: %.2f%%",
														linphone_call_stats_get_sender_loss_rate(stats)]];
		[result appendString:@"\n"];
		[result appendString:[NSString stringWithFormat:@"Receiver loss rate: %.2f%%",
														linphone_call_stats_get_receiver_loss_rate(stats)]];
		[result appendString:@"\n"];

		if (stream == LinphoneStreamTypeVideo) {
			MSVideoSize sentSize = linphone_call_params_get_sent_video_size(params);
			MSVideoSize recvSize = linphone_call_params_get_received_video_size(params);
			float sentFPS = linphone_call_params_get_sent_framerate(params);
			float recvFPS = linphone_call_params_get_received_framerate(params);
			[result appendString:[NSString stringWithFormat:@"Sent video resolution: %dx%d (%.1fFPS)", sentSize.width,
															sentSize.height, sentFPS]];
			[result appendString:@"\n"];
			[result appendString:[NSString stringWithFormat:@"Received video resolution: %dx%d (%.1fFPS)",
															recvSize.width, recvSize.height, recvFPS]];
			[result appendString:@"\n"];
		}
	}
	return result;
}

- (void)updateStats:(NSTimer *)timer {
	LinphoneCall *call = linphone_core_get_current_call(LC);

	if (!call) {
		_statsLabel.text = NSLocalizedString(@"No call in progress", nil);
		return;
	}

	NSMutableString *stats = [[NSMutableString alloc] init];

	LinphoneMediaEncryption enc = linphone_call_params_get_media_encryption(linphone_call_get_current_params(call));
	if (enc != LinphoneMediaEncryptionNone) {
		[stats appendString:[NSString
								stringWithFormat:@"Call encrypted using %@", [self.class mediaEncryptionToString:enc]]];
	}

	[stats appendString:[self updateStatsForCall:call stream:LinphoneStreamTypeAudio]];
	[stats appendString:[self updateStatsForCall:call stream:LinphoneStreamTypeVideo]];
	[stats appendString:[self updateStatsForCall:call stream:LinphoneStreamTypeText]];

	_statsLabel.text = stats;
}

@end
