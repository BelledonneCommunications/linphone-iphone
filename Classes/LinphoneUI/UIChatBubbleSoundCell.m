//
//  UIChatBubbleSoundCell.m
//  linphone
//
//  Created by David Idmansour on 02/07/2018.
//

#import "UIChatBubbleSoundCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVKit/AVKit.h>

@implementation UIChatBubbleSoundCell {
    ChatConversationTableView *chatTableView;
}

#pragma mark - Lifecycle Functions

- (void)awakeFromNib {
    [super awakeFromNib];
    _player = linphone_core_create_local_player(LC, NULL, NULL, NULL);
    _cbs = linphone_player_get_callbacks(_player);
    linphone_player_cbs_set_eof_reached(_cbs, on_eof_reached);
    linphone_player_cbs_set_user_data(_cbs, (__bridge void*)self);
    _fileName = [LinphoneManager bundleFile:@"hold.mkv"];
    NSLog(@"Opening sound file %@", _fileName);
    linphone_player_open(_player, _fileName.UTF8String);
    _duration = linphone_player_get_duration(_player);
    _durationString = [UIChatBubbleSoundCell timeToString:_duration];
    [self updateTimeLabel:0];
    NSLog(@"Duration : %@", _durationString);
}

- (void)updateTimeLabel:(int)currentTime {
    _timeLabel.text = [NSString stringWithFormat:@"%@ / %@", [UIChatBubbleSoundCell timeToString:currentTime], _durationString];
}

- (id)initWithIdentifier:(NSString *)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        // TODO: remove text cell subview
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        // resize cell to match .nib size. It is needed when resized the cell to
        // correctly adapt its height too
        UIView *sub = nil;
        for (int i = 0; i < arrayOfViews.count; i++) {
            if ([arrayOfViews[i] isKindOfClass:UIView.class]) {
                sub = arrayOfViews[i];
                break;
            }
        }
        [self addSubview:sub];
        chatTableView = VIEW(ChatConversationView).tableController;
    }
    return self;
}

- (void)goToBeginning {
    linphone_player_pause(_player);
    linphone_player_seek(_player, 0);
}

void on_eof_reached(LinphonePlayer *pl) {
    NSLog(@"End of file reached");
    linphone_player_seek(pl, 0);
}

- (void)update:(LinphonePlayer *)pl {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        while (linphone_player_get_state(pl) == LinphonePlayerPlaying) {
            int start = linphone_player_get_current_position(pl);
            while (start + 1000 < _duration && start + 1000 > linphone_player_get_current_position(pl)) {
                
            }
            start = linphone_player_get_current_position(pl);
            dispatch_async(dispatch_get_main_queue(), ^{
                _timeProgressBar.progress = (float)start / (float)_duration;
                [self updateTimeLabel:start];
            });
        }
    });
}

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

- (IBAction)onPlay:(id)sender {
    if (_player) {
        LinphonePlayerState state = linphone_player_get_state(_player);
        switch(state) {
            case LinphonePlayerPlaying:
                NSLog(@"Pausing");
                linphone_player_pause(_player);
                break;
            case LinphonePlayerClosed:
                NSLog(@"Opening file");
                linphone_player_open(_player, _fileName.UTF8String);
            case LinphonePlayerPaused:
                NSLog(@"Playing");
                linphone_player_start(_player);
                break;
        }
        [self update:_player];
    } else {
        NSLog(@"Error");
    }
}

- (IBAction)onStop:(id)sender {
    if (_player) {
        NSLog(@"Stopping");
        [self goToBeginning];
        dispatch_async(dispatch_get_main_queue(), ^{
           _timeProgressBar.progress = 0;
            [self updateTimeLabel:0];
        });
    } else {
        NSLog(@"Error");
    }
}
@end
