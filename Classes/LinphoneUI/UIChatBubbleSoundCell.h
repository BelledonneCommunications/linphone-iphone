//
//  UIChatBubbleSoundCell.h
//  linphone
//
//  Created by David Idmansour on 02/07/2018.
//

#import "UIChatBubbleTextCell.h"

#import "UILoadingImageView.h"
#import "UITextViewNoDefine.h"
#import "FileTransferDelegate.h"
#import "ChatConversationTableView.h"
#import "UIChatBubbleTextCell.h"

@interface UIChatBubbleSoundCell : UIChatBubbleTextCell
@property (weak, nonatomic) IBOutlet UIProgressView *timeProgressBar;
@property (weak, nonatomic) IBOutlet UILabel *timeLabel;
@property (weak, nonatomic) IBOutlet UIButton *playPauseButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property LinphonePlayer *player;
@property LinphonePlayerCbs *cbs;
@property (strong, nonatomic) NSString *fileName;
@property (strong, nonatomic) NSString *durationString;
@property int duration;
@property (weak, nonatomic) IBOutlet UIView *playerView;
@property BOOL shouldClosePlayer;
@property BOOL eofReached;
@property (weak, nonatomic) IBOutlet UIButton *loadButton;
@property (strong, nonatomic) IBOutlet UITapGestureRecognizer *seekRecognizer;


- (IBAction)onPlay:(id)sender;
- (IBAction)onStop:(id)sender;
- (IBAction)onLoad:(id)sender;
- (void)updateTimeLabel:(int)currentTime;

@end
