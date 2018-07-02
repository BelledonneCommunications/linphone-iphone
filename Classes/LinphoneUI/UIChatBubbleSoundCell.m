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
    LinphonePlayer *player;
}

#pragma mark - Lifecycle Functions

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
        player = linphone_core_create_local_player(LC, NULL, NULL, (__bridge void*)self.window);
    }
    return self;
}

- (IBAction)onPlay:(id)sender {
    if (!player) {
        player = linphone_core_create_local_player(LC, NULL, NULL, NULL);
    }
    if (player) {
        if (!soundIsOpened) {
            NSLog(@"Opening sound file");
            linphone_player_open(player, [LinphoneManager bundleFile:@"ringback.wav"].UTF8String);
            soundIsOpened = TRUE;
        }
        NSLog(@"Playing");
        linphone_player_start(player);
    } else {
        NSLog(@"Error");
    }
}

- (IBAction)onPause:(id)sender {
    if (player) {
        NSLog(@"Pausing");
        linphone_player_pause(player);
    } else {
        NSLog(@"Error");
    }
}

- (IBAction)onStop:(id)sender {
    if (player) {
        NSLog(@"Closing file");
        linphone_player_close(player);
        soundIsOpened = FALSE;
    } else {
        NSLog(@"Error");
    }
}
@end
