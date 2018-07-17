//
//  UISoundRecordView.m
//  linphone
//
//  Created by David Idmansour on 09/07/2018.
//

#import "UISoundRecordView.h"
#import "PhoneMainView.h"
#import "UILinphoneAudioPlayer.h"
#import "Utils.h"

@implementation UISoundRecordView {
    @private
    LinphoneRecordState state;
    NSString *durationString;
    NSString *file;
    UILinphoneAudioPlayer *player;
}

#pragma mark - Lifecycle

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
        [_cancelButton setTitle:@"" forState:UIControlStateNormal];
        [_cancelButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemTrash:[UIColor blackColor]] forState:UIControlStateNormal];
        [_cancelButton2 setTitle:@"" forState:UIControlStateNormal];
        [_cancelButton2 setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemTrash:[UIColor blackColor]] forState:UIControlStateNormal];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(pausePlayer:)
                                                     name:UIApplicationWillResignActiveNotification
                                                   object:nil];
    }
    return self;
}

- (void)closePlayer {
    [player close];
}

- (void)reset {
    state = LinphoneRecordNotStarted;
    [_recordButton setTitle:@"Record" forState:UIControlStateNormal];
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

- (void)pausePlayer:(NSNotification *)notif {
    [player pause];
}

#pragma mark - Event handling

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
    // replace with real file
    file = [LinphoneManager bundleFile:@"hold.mkv"];
    // moving to sending view
    _sendingView.frame = _recordView.frame;
    _sendingView.bounds = _recordView.bounds;
    player = [UILinphoneAudioPlayer audioPlayerWithFilePath:file];
    player.view.frame = _playerView.frame;
    player.view.bounds = _playerView.bounds;
    [_playerView addSubview:player.view];
    [player open];
    [_recordView.superview addSubview:_sendingView];
    [_recordView removeFromSuperview];
}

- (IBAction)onCancel:(UIButton *)sender {
    LOGI(@"Canceled recording");
    [VIEW(ChatConversationView) changeToMessageView];
    [self reset];
    [self closePlayer];
}

- (IBAction)onSend:(UIButton *)sender {
    LOGI(@"Sending audio message...");
    [VIEW(ChatConversationView) startFileUpload:[NSData dataWithContentsOfFile:file]
                                        withUrl:[NSURL URLWithString:file]];
    [VIEW(ChatConversationView) changeToMessageView];
    [self reset];
    [self closePlayer];
}

@end
