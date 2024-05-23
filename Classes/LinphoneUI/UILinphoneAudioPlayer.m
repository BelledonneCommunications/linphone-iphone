/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "UILinphoneAudioPlayer.h"
#import "Utils.h"
#import "PhoneMainView.h"

@implementation UILinphoneAudioPlayer {
    @private
    LinphonePlayer *player;
    LinphonePlayerCbs *cbs;
    int duration;
    BOOL eofReached;
}

@synthesize file;

#pragma mark - Factory

+ (id)audioPlayerWithFilePath:(NSString *)filePath {
    return [[self alloc] initWithFilePath:filePath];
}

#pragma mark - Life cycle

- (instancetype)initWithFilePath:(NSString *)filePath {
    if (self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]]) {
        player = linphone_core_create_local_player(LC, NULL, "IOSDisplay", NULL);
        cbs = linphone_factory_create_player_cbs(linphone_factory_get());
        linphone_player_set_user_data(player, (__bridge void *)self);
        linphone_player_cbs_set_eof_reached(cbs, on_eof_reached);
        file = filePath;
        eofReached = NO;
		_refreshTimer = nil;
    }
    return self;
}

- (void)dealloc {
    [self close];
}

- (void)close {
    if (player) {
		[_refreshTimer invalidate];
		_refreshTimer = nil;
		linphone_player_close(player);
        linphone_player_unref(player);
        player = NULL;
    }
    [self.view removeFromSuperview];
}

- (void)viewDidAppear:(BOOL)animated {
    [_playButton setTitle:@"" forState:UIControlStateNormal];
    if (player && linphone_player_get_state(player) == LinphonePlayerPlaying)
        [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPause:[UIColor blackColor]] forState:UIControlStateNormal];
    else
        [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPlay:[UIColor blackColor]] forState:UIControlStateNormal];
    [_stopButton setTitle:@"" forState:UIControlStateNormal];
    [_stopButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemRefresh:[UIColor blackColor]] forState:UIControlStateNormal];
}

- (void)viewWillDisappear:(BOOL)animated {
	[self close];
}

- (void)open {
    linphone_player_open(player, file.UTF8String);
    duration = linphone_player_get_duration(player);
    [self updateTimeLabel:0];
    _timeProgress.progress = 0;
	
    eofReached = NO;
    [_playButton setTitle:@"" forState:UIControlStateNormal];
    [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPlay:[UIColor blackColor]] forState:UIControlStateNormal];
    [_stopButton setTitle:@"" forState:UIControlStateNormal];
    [_stopButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemRefresh:[UIColor blackColor]] forState:UIControlStateNormal];
}

- (BOOL)isOpened {
    return player && linphone_player_get_state(player) != LinphonePlayerClosed;
}

- (BOOL)isCreated {
	return player!=nil;
}

- (void)setFile:(NSString *)fileName {
    if (player) linphone_player_close(player);
    file = fileName;
}

#pragma mark - Callbacks

void on_eof_reached(LinphonePlayer *pl) {
    NSLog(@"EOF reached");
    UILinphoneAudioPlayer *player = (__bridge UILinphoneAudioPlayer *)linphone_player_get_user_data(pl);
    dispatch_async(dispatch_get_main_queue(), ^{
        [player.playButton setTitle:@"" forState:UIControlStateNormal];
        [player.playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPlay:[UIColor blackColor]] forState:UIControlStateNormal];
    });
    player->eofReached = YES;
	VIEW(RecordingsListView).videoView.hidden = YES;
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

- (void)displayProgress{
	int pos = linphone_player_get_current_position(player);
	_timeProgress.progress = (float)pos / (float)duration;
	[self updateTimeLabel:pos];
}

- (void)update {
	if (!_refreshTimer)
		_refreshTimer = [NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(displayProgress) userInfo:nil repeats:YES];
}

- (void)pause {
    if ([self isOpened]) {
        linphone_player_pause(player);
        [_playButton setTitle:@"" forState:UIControlStateNormal];
        [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPlay:[UIColor blackColor]] forState:UIControlStateNormal];
    }
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
            [_playButton setTitle:@"" forState:UIControlStateNormal];
            [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPause:[UIColor blackColor]] forState:UIControlStateNormal];
            linphone_player_start(player);
			if (linphone_player_get_is_video_available(player)) {
				linphone_player_set_window_id(player, (__bridge void *)VIEW(RecordingsListView).videoView);
				VIEW(RecordingsListView).videoView.hidden = NO;
			} else {
				VIEW(RecordingsListView).videoView.hidden = YES;
			}
            break;
        case LinphonePlayerPlaying:
            NSLog(@"Pause");
            [_playButton setTitle:@"" forState:UIControlStateNormal];
            [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPlay:[UIColor blackColor]] forState:UIControlStateNormal];
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
    [_playButton setTitle:@"" forState:UIControlStateNormal];
    [_playButton setImage:[UIImage imageFromSystemBarButton:UIBarButtonSystemItemPlay:[UIColor blackColor]] forState:UIControlStateNormal];
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
