//
//  UIChatBubbleSoundCell.m
//  linphone
//
//  Created by David Idmansour on 02/07/2018.
//

#import "UIChatBubbleSoundCell.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVKit/AVKit.h>

@implementation UIChatBubbleSoundCell {
    ChatConversationTableView *chatTableView;
    UILinphoneAudioPlayer *audioPlayer;
}

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
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

#pragma mark - Getters & setters

- (void)setAudioPlayer:(UILinphoneAudioPlayer *)player {
    if (!player)
        return;
    if (audioPlayer)
        [audioPlayer.view removeFromSuperview];
    audioPlayer = player;
    [UILinphoneAudioPlayer registerPlayer:player forMessage:self.message];
    audioPlayer.view.hidden = YES;
    _loadButton.hidden = NO;
    [_content addSubview:audioPlayer.view];
    [self bringSubviewToFront:audioPlayer.view];
    audioPlayer.view.frame = _playerView.frame;
    audioPlayer.view.bounds = _playerView.bounds;
    _playerView.hidden = YES;
}

#pragma mark - Event handlers

- (IBAction)onLoad:(id)sender {
    if (!audioPlayer || audioPlayer != [UILinphoneAudioPlayer playerForMessage:self.message]) {
        [audioPlayer.view removeFromSuperview];
        NSString *localFile = [LinphoneManager getMessageAppDataForKey:@"localsound" inMessage:self.message];
        NSString *filePath = [LinphoneManager documentFile:localFile];
        audioPlayer = [UILinphoneAudioPlayer audioPlayerWithFilePath:filePath];
        [UILinphoneAudioPlayer registerPlayer:audioPlayer forMessage:self.message];
        [_content addSubview:audioPlayer.view];
        audioPlayer.view.frame = _playerView.frame;
        audioPlayer.view.bounds = _playerView.bounds;
    }
    _loadButton.hidden = YES;
    audioPlayer.view.hidden = NO;
    [audioPlayer open];
}

- (IBAction)onResend:(id)sender {
    [super onResend];
}

#pragma mark - Overriden methods

- (void)setEvent:(LinphoneEventLog *)event {
    if (!event || !(linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage))
        return;
    
    super.event = event;
    [self setChatMessage:linphone_event_log_get_chat_message(event)];
    LinphoneContent *content = linphone_chat_message_get_file_transfer_information(self.message);
    NSLog(@"File name : %s", linphone_content_get_name(content));
    NSLog(@"Type : %s", linphone_content_get_type(content));
    if (audioPlayer)
        [audioPlayer.view removeFromSuperview];
    audioPlayer = [UILinphoneAudioPlayer playerForMessage:self.message];
    if (audioPlayer && [audioPlayer isOpened]) {
        audioPlayer.view.hidden = NO;
        _loadButton.hidden = YES;
    } else {
        audioPlayer.view.hidden = YES;
        _loadButton.hidden = NO;
    }
    [_content addSubview:audioPlayer.view];
    [self bringSubviewToFront:audioPlayer.view];
    audioPlayer.view.frame = _playerView.frame;
    audioPlayer.view.bounds = _playerView.bounds;
    _playerView.hidden = YES;
}

@end
