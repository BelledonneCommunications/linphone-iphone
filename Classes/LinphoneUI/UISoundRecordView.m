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
    LinphonePlayer *player;
    LinphonePlayerCbs *cbs;
    int duration;
    BOOL shouldClosePlayer;
    NSString *durationString;
    NSString *file;
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

- (void)initPlayer {
    player = linphone_core_create_local_player(LC, NULL, NULL, NULL);
    cbs = linphone_player_get_callbacks(player);
    file = [LinphoneManager bundleFile:@"hold.mkv"]; //tmp
    linphone_player_open(player, file.UTF8String);
    duration = linphone_player_get_duration(player);
    durationString = [self.class timeToString:duration];
    shouldClosePlayer = NO;
}

- (void)closePlayer {
    if (!player)
        return;
    if (linphone_player_get_state(player) == LinphonePlayerPlaying) {
        shouldClosePlayer = YES;
        return;
    }
    linphone_player_close(player);
    linphone_player_unref(player);
    player = NULL;
    cbs = NULL;
}

#pragma mark - Utils

+ (NSString *)timeToString:(int)time {
    time /= 1000;
    int hours = time / 3600;
    time %= 3600;
    int minutes = time / 60;
    int seconds = time % 60;
    NSNumberFormatter *formatter = [NSNumberFormatter new];
    formatter.maximumIntegerDigits = 2;
    formatter.minimumIntegerDigits = 2;
    NSString *ret = [NSString stringWithFormat:@"%@:%@",
                     [formatter stringFromNumber:[NSNumber numberWithInt:minutes]],
                     [formatter stringFromNumber:[NSNumber numberWithInt:seconds]]];
    ret = (hours == 0)?ret:[[NSString stringWithFormat:@"%d:", hours] stringByAppendingString:ret];
    return ret;
}

- (void)updateTimeLabel:(int)currentTime {
    _timeLabel.text = [NSString stringWithFormat:@"%@ / %@", [self.class timeToString:currentTime], durationString];
}

- (void)update:(LinphonePlayer *)pl {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        while (linphone_player_get_state(pl) == LinphonePlayerPlaying) {
            int start = linphone_player_get_current_position(pl);
            while (start + 1000 < duration && start + 1000 > linphone_player_get_current_position(pl)) {
                [NSThread sleepForTimeInterval:0.01];
                if (linphone_player_get_state(pl) == LinphonePlayerPaused)
                    break;
            }
            start = linphone_player_get_current_position(pl);
            dispatch_async(dispatch_get_main_queue(), ^{
                _timeBar.progress = (float)start / (float)duration;
                [self updateTimeLabel:start];
            });
            if (shouldClosePlayer)
                break;
        }
        if (shouldClosePlayer) {
            linphone_player_close(player);
            linphone_player_unref(player);
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateTimeLabel:0];
                _timeBar.progress = 0;
                [_playButton setTitle:@"Play" forState:UIControlStateNormal];
            });
            player = NULL;
            cbs = NULL;
        }
    });
}

- (void)reset {
    state = LinphoneRecordNotStarted;
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
    _sendingView.frame = _recordView.frame;
    _sendingView.bounds = _recordView.bounds;
    [_recordView.superview addSubview:_sendingView];
    [_recordView removeFromSuperview];
    [self initPlayer];
}

- (IBAction)onCancel:(UIButton *)sender {
    LOGI(@"Canceled recording");
    [VIEW(ChatConversationView) changeToMessageView];
    [self reset];
    [self closePlayer];
}

- (IBAction)onSend:(UIButton *)sender {
    LOGI(@"Sending audio message...");
    NSData *data = [NSData dataWithContentsOfFile:[LinphoneManager bundleFile:@"hold.mkv"]];
    NSLog(@"Size of data : %d", (unsigned int)data.length);
    [VIEW(ChatConversationView) startFileUpload:[NSData dataWithContentsOfFile:[LinphoneManager bundleFile:@"hold.mkv"]] withUrl:[NSURL URLWithString:[@"file://" stringByAppendingString:[LinphoneManager bundleFile:@"hold.mkv"]]]];
    [VIEW(ChatConversationView) changeToMessageView];
    [self reset];
    [self closePlayer];
}

- (IBAction)onPlay:(UIButton *)sender {
    switch (linphone_player_get_state(player)) {
        case LinphonePlayerPaused:
            LOGI(@"Play");
            [_playButton setTitle:@"Pause" forState:UIControlStateNormal];
            linphone_player_start(player);
            [self update:player];
            break;
        case LinphonePlayerPlaying:
            [_playButton setTitle:@"Play" forState:UIControlStateNormal];
            LOGI(@"Pause");
            linphone_player_pause(player);
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
