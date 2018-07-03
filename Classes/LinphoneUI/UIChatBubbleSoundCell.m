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
    BOOL soundIsOpened;
    BOOL soundIsPlaying;
    BOOL eofReached;
}

#pragma mark - Lifecycle Functions

- (void)awakeFromNib {
    [super awakeFromNib];
    _player = linphone_core_create_local_player(LC, NULL, NULL, NULL);
    _cbs = linphone_player_get_callbacks(_player);
    linphone_player_cbs_set_eof_reached(_cbs, on_eof_reached);
    soundIsOpened = FALSE;
    soundIsPlaying = FALSE;
    eofReached = FALSE;
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
        soundIsOpened = FALSE;
        soundIsPlaying = FALSE;
        _player = linphone_core_create_local_player(LC, NULL, NULL, NULL);
    }
    return self;
}

void on_eof_reached(LinphonePlayer *pl) {
    NSLog(@"End of file reached");
    NSLog(linphone_player_get_state(pl) == LinphonePlayerPaused?@"Closed":@"Not closed");
    NSLog(@"Duration : %@", [UIChatBubbleSoundCell timeToString:linphone_player_get_duration(pl)]);
}

- (void)update:(LinphonePlayer *)pl {
    while (soundIsPlaying && !eofReached) {
        int duration = linphone_player_get_duration(pl);
        int currentTime = linphone_player_get_current_position(pl);
        dispatch_async(dispatch_get_main_queue(), ^{
            _timeProgressBar.progress = (float)currentTime / (float)duration;
        });
        soundIsPlaying = linphone_player_get_state(_player) == LinphonePlayerPlaying;
    }
    
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
        if (!soundIsOpened) {
            NSLog(@"Opening sound file");
            linphone_player_open(_player, [LinphoneManager bundleFile:@"hold.mkv"].UTF8String);
            NSLog(@"Duration : %@", [UIChatBubbleSoundCell timeToString:linphone_player_get_duration(_player)]);
            soundIsOpened = TRUE;
        }
        if (!soundIsPlaying) {
            NSLog(@"Playing");
            linphone_player_start(_player);
        } else {
            NSLog(@"Pausing");
            linphone_player_pause(_player);
        }
        soundIsPlaying = !soundIsPlaying;
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            [self update:_player];
        });
    } else {
        NSLog(@"Error");
    }
}

- (IBAction)onPause:(id)sender {
    if (_player) {
        NSLog(@"Pausing");
        linphone_player_pause(_player);
    } else {
        NSLog(@"Error");
    }
}

- (IBAction)onStop:(id)sender {
    if (_player) {
        NSLog(@"Closing file");
        linphone_player_close(_player);
        soundIsOpened = FALSE;
        soundIsPlaying = FALSE;
        _timeProgressBar.progress = 0;
    } else {
        NSLog(@"Error");
    }
}
@end
