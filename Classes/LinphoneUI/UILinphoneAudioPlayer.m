//
//  UILinphoneAudioPlayer.m
//  linphone
//
//  Created by David Idmansour on 13/07/2018.
//

#import "UILinphoneAudioPlayer.h"

@implementation UILinphoneAudioPlayer {
    @private
    LinphonePlayer *player;
    LinphonePlayerCbs *cbs;
    NSString *file;
    int duration;
    BOOL eofReached;
}

#pragma mark - Factory
static NSMutableDictionary *players;

+ (void)registerPlayer:(UILinphoneAudioPlayer *)aPlayer forMessage:(LinphoneChatMessage *)msg {
    @synchronized(self) {
        if (!players)
            players = [NSMutableDictionary dictionary];
        [players setObject:aPlayer forKey:[NSValue valueWithPointer:msg]];
    }
}

+ (UILinphoneAudioPlayer *)playerForMessage:(LinphoneChatMessage *)msg {
    @synchronized(self) {
        return [players objectForKey:[NSValue valueWithPointer:msg]];
    }
}

+ (id)audioPlayerWithFilePath:(NSString *)filePath {
    return [[self alloc] initWithFilePath:filePath];
}

+ (void)closePlayers {
    @synchronized(self) {
        for (UILinphoneAudioPlayer *p in [players allValues]) {
            [p close];
        }
        [players removeAllObjects];
    }
}

#pragma mark - Life cycle

- (instancetype)initWithFilePath:(NSString *)filePath {
    if (self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]]) {
        player = linphone_core_create_local_player(LC, NULL, NULL, NULL);
        cbs = linphone_player_get_callbacks(player);
        linphone_player_set_user_data(player, (__bridge void *)self);
        linphone_player_cbs_set_eof_reached(cbs, on_eof_reached);
        file = filePath;
        eofReached = NO;
    }
    return self;
}

- (void)dealloc {
    [self close];
}

- (void)close {
    if (player) {
        linphone_player_unref(player);
        player = NULL;
    }
    [self.view removeFromSuperview];
}

- (void)open {
    linphone_player_open(player, file.UTF8String);
    duration = linphone_player_get_duration(player);
    [self updateTimeLabel:0];
}

- (BOOL)isOpened {
    return player && linphone_player_get_state(player) != LinphonePlayerClosed;
}

#pragma mark - Callbacks

void on_eof_reached(LinphonePlayer *pl) {
    NSLog(@"EOF reached");
    UILinphoneAudioPlayer *player = (__bridge UILinphoneAudioPlayer *)linphone_player_get_user_data(pl);
    dispatch_async(dispatch_get_main_queue(), ^{
        [player.playButton setTitle:@"Play" forState:UIControlStateNormal];
    });
    player->eofReached = YES;
}

#pragma mark - ViewController methods

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
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


#pragma mark - Updating

- (void)updateTimeLabel:(int)currentTime {
    _timeLabel.text = [NSString stringWithFormat:@"%@ / %@", [self.class timeToString:currentTime], [self.class timeToString:duration]];
}

- (void)update {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        while (player && linphone_player_get_state(player) == LinphonePlayerPlaying) {
            int start = linphone_player_get_current_position(player);
            while (player && start + 100 < duration && start + 100 > linphone_player_get_current_position(player)) {
                [NSThread sleepForTimeInterval:0.01];
                if (!player || linphone_player_get_state(player) == LinphonePlayerPaused)
                    break;
            }
            start = player ? linphone_player_get_current_position(player) : start;
            dispatch_async(dispatch_get_main_queue(), ^{
                _timeProgress.progress = (float)start / (float)duration;
                [self updateTimeLabel:start];
            });
        }
    });
}

#pragma mark - Event handlers

- (IBAction)onPlay:(id)sender {
    if (eofReached) {
        linphone_player_seek(player, 0);
        eofReached = NO;
    }
    LinphonePlayerState state = linphone_player_get_state(player);
    switch (state) {
        case LinphonePlayerClosed:
            break;
        case LinphonePlayerPaused:
            NSLog(@"Play");
            [_playButton setTitle:@"Pause" forState:UIControlStateNormal];
            linphone_player_start(player);
            break;
        case LinphonePlayerPlaying:
            NSLog(@"Pause");
            [_playButton setTitle:@"Play" forState:UIControlStateNormal];
            linphone_player_pause(player);
            break;
    }
    [self update];
}

- (IBAction)onStop:(id)sender {
    NSLog(@"Stop");
    linphone_player_pause(player);
    linphone_player_seek(player, 0);
    eofReached = NO;
    [_playButton setTitle:@"Play" forState:UIControlStateNormal];
    _timeProgress.progress = 0;
    [self updateTimeLabel:0];
}

- (IBAction)onTapTimeBar:(UITapGestureRecognizer *)sender {
    if (sender.state != UIGestureRecognizerStateEnded)
        return;
    CGPoint loc = [sender locationInView:self.view];
    CGPoint timeLoc = _timeProgress.frame.origin;
    CGSize timeSize = _timeProgress.frame.size;
    if (loc.x >= timeLoc.x && loc.x <= timeLoc.x + timeSize.width && loc.y >= timeLoc.y - 10 && loc.y <= timeLoc.y + timeSize.height + 10) {
        float progress = (loc.x - timeLoc.x) / timeSize.width;
        _timeProgress.progress = progress;
        [self updateTimeLabel:(int)(progress * duration)];
        linphone_player_seek(player, (int)(progress * duration));
    }
}
@end
