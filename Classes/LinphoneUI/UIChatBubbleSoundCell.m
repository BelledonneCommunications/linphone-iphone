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
}

static UILinphoneAudioPlayer *player;
static LinphoneChatMessage *currentMessage;

+ (void)acquirePlayerForMessage:(LinphoneChatMessage *)msg {
    @synchronized(self) {
        if (!player)
            player = [UILinphoneAudioPlayer audioPlayerWithFilePath:[LinphoneManager documentFile:[LinphoneManager getMessageAppDataForKey:@"localsound" inMessage:msg]]];
        else
            [player setFile:[LinphoneManager documentFile:[LinphoneManager getMessageAppDataForKey:@"localsound" inMessage:msg]]];
        currentMessage = msg;
    }
}

+ (UILinphoneAudioPlayer *)player {
    @synchronized(self) {
        return player;
    }
}

+ (LinphoneChatMessage *)currentMessage {
    @synchronized(self) {
        return currentMessage;
    }
}

+ (void)resetBubblesUI {
    ChatConversationTableView *chatTableView = VIEW(ChatConversationView).tableController;
    for (UITableViewCell *c in chatTableView.tableView.visibleCells) {
        if (![c isMemberOfClass:self.class])
            continue;
        UIChatBubbleSoundCell *cell = (UIChatBubbleSoundCell *)c;
        if (cell.message != [self currentMessage])
            cell.loadButton.hidden = NO;
    }
}

+ (void)closeAudioPlayer {
    @synchronized(self) {
        if (player)
            [player close];
        player = nil;
        currentMessage = NULL;
    }
    [self resetBubblesUI];
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

#pragma mark - Event handlers

- (IBAction)onLoad:(id)sender {
    [self.class acquirePlayerForMessage:self.message];
    [self.class resetBubblesUI];
    UILinphoneAudioPlayer *p = [self.class player];
    [p.view removeFromSuperview];
    [_content addSubview:p.view];
    [self bringSubviewToFront:p.view];
    p.view.frame = _playerView.frame;
    p.view.bounds = _playerView.bounds;
    _loadButton.hidden = YES;
    p.view.hidden = NO;
    [p open];
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
    UILinphoneAudioPlayer *p = [self.class player];
    if ([self.class currentMessage] == self.message) {
        [p.view removeFromSuperview];
        [_content addSubview:p.view];
        [self bringSubviewToFront:p.view];
        p.view.frame = _playerView.frame;
        p.view.bounds = _playerView.bounds;
        _loadButton.hidden = YES;
        p.view.hidden = NO;
    } else {
        _loadButton.hidden = NO;
        // self could be the same cell instance than the one used
        // for the current active player
        // so the player view needs to be hidden
        // since it shouldn't be displayed here
        if ([_content.subviews containsObject:p.view])
            p.view.hidden = YES;
    }
    _playerView.hidden = YES;
}

@end
