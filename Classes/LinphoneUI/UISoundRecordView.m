//
//  UISoundRecordView.m
//  linphone
//
//  Created by David Idmansour on 09/07/2018.
//

#import "UISoundRecordView.h"
#import "PhoneMainView.h"

@implementation UISoundRecordView {
    @private
    LinphoneRecordState state;
    LinphonePlayerState playerState;
}

- (id)init {
    if (self = [super init]) {
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        UIView *sub = nil;
        for (int i = 0; i < arrayOfViews.count; i++) {
            if ([arrayOfViews[i] isKindOfClass:UIView.class]) {
                sub = arrayOfViews[i];
                break;
            }
        }
        [self addSubview:sub];
        self.recordView = sub;
        [self reset];
    }
    return self;
}

- (void)reset {
    state = LinphoneRecordNotStarted;
    playerState = LinphonePlayerPaused;
    [_recordButton setTitle:@"Record" forState:UIControlStateNormal];
    [_playButton setTitle:@"Play" forState:UIControlStateNormal];
    _timeBar.progress = 0;
}

- (IBAction)onRecord:(UIButton *)sender {
    switch(state) {
        case LinphoneRecording:
            LOGI(@"Record paused");
            [_recordButton setTitle:@"Record" forState:UIControlStateNormal];
            state = LinphoneRecordPaused;
            break;
        case LinphoneRecordPaused:
            LOGI(@"Recording...");
            [_recordButton setTitle:@"Pause" forState:UIControlStateNormal];
            state = LinphoneRecording;
            break;
        case LinphoneRecordFinished:
            LOGI(@"Restarted recording, erasing previous record...");
            [_recordButton setTitle:@"Record" forState:UIControlStateNormal];
            state = LinphoneRecording;
            break;
        case LinphoneRecordNotStarted:
            LOGI(@"Starting recording...");
            [_recordButton setTitle:@"Pause" forState:UIControlStateNormal];
            state = LinphoneRecording;
            break;
    }
}

- (IBAction)onStop:(UIButton *)sender {
    if (state == LinphoneRecordNotStarted) {
        LOGI(@"Can't stop a recording that has not been started");
        return;
    }
    LOGI(@"Stopped recording");
    state = LinphoneRecordFinished;
    // moving to sending view
    playerState = LinphonePlayerPaused;
    _sendingView.frame = _recordView.frame;
    _sendingView.bounds = _recordView.bounds;
    [_recordView.superview addSubview:_sendingView];
    [_recordView removeFromSuperview];
}

- (IBAction)onCancel:(UIButton *)sender {
    LOGI(@"Canceled recording");
    [VIEW(ChatConversationView) changeToMessageView];
}

- (IBAction)onSend:(UIButton *)sender {
    LOGI(@"Sending audio message...");
    NSData *data = [NSData dataWithContentsOfFile:[LinphoneManager bundleFile:@"hold.mkv"]];
    NSLog(@"Size of data : %d", (unsigned int)data.length);
    [VIEW(ChatConversationView) startFileUpload:[NSData dataWithContentsOfFile:[LinphoneManager bundleFile:@"hold.mkv"]] withUrl:[NSURL URLWithString:[@"file://" stringByAppendingString:[LinphoneManager bundleFile:@"hold.mkv"]]]];
    [VIEW(ChatConversationView) changeToMessageView];
}

- (IBAction)onPlay:(UIButton *)sender {
    switch (playerState) {
        case LinphonePlayerPaused:
            LOGI(@"Play");
            [_playButton setTitle:@"Pause" forState:UIControlStateNormal];
            playerState = LinphonePlayerPlaying;
            break;
        case LinphonePlayerPlaying:
            [_playButton setTitle:@"Play" forState:UIControlStateNormal];
            LOGI(@"Pause");
            playerState = LinphonePlayerPaused;
        default:
            break;
    }
}

- (IBAction)onTapBar:(UITapGestureRecognizer *)sender {
    if (sender.state != UIGestureRecognizerStateEnded)
        return;
    CGPoint loc = [sender locationInView:_sendingView];
    CGPoint timeLoc = _timeBar.frame.origin;
    CGSize timeSize = _timeBar.frame.size;
    if (loc.x >= timeLoc.x && loc.x <= timeLoc.x + timeSize.width && loc.y >= timeLoc.y - 10 && loc.y <= timeLoc.y + timeSize.height + 10) {
        float progress = (loc.x - timeLoc.x) / timeSize.width;
        _timeBar.progress = progress;
    }
}

@end
