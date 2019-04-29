//
//  UIRecordingCell.m
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import "UIRecordingCell.h"
#import "PhoneMainView.h"
#import "UILabel+Boldify.h"
#import "Utils.h"
#import "UILinphoneAudioPlayer.h"

@implementation UIRecordingCell

static UILinphoneAudioPlayer *player;

#pragma mark - Lifecycle Functions
/*
 * TODO:
 * - When we scroll past a selected row, the player loads incorrectly (no buttons). Probably a problem in the player code.
 * - mkv recording is probably buggy, wrong eof. wav playing works but does not display the length/timestamp.
 * - The share button is greyed out when not clicking it. idk why, it's really weird.
*/
- (id)initWithIdentifier:(NSString *)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier])) {
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        
        // resize cell to match .nib size. It is needed when resized the cell to
        // correctly adapt its height too
        UIRecordingCell *sub = [arrayOfViews objectAtIndex:0];
        [self setFrame:CGRectMake(0, 0, sub.frame.size.width, 40)];
        self = sub;
        self.recording = NULL;
        _shareButton.target = self;
        _shareButton.action = @selector(onShareButtonPressed);
    }
    return self;
}

- (void)dealloc {
    self.recording = NULL;
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Property Functions

- (void)setRecording:(NSString *)arecording {
    _recording = arecording;
    if(_recording) {
        NSArray *parsedRecording = [LinphoneUtils parseRecordingName:_recording];
        NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
        [dateFormat setDateFormat:@"HH:mm:ss"];
        _nameLabel.text = [[[parsedRecording objectAtIndex:0] stringByAppendingString:@" @ "] stringByAppendingString:[dateFormat stringFromDate:[parsedRecording objectAtIndex:1]]];
    }
}

#pragma mark -

- (NSString *)accessibilityLabel {
    return _nameLabel.text;
}

- (void)setEditing:(BOOL)editing {
    [self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    if (animated) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:0.3];
    }
    if (animated) {
        [UIView commitAnimations];
    }
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated {
    self.selectionStyle = UITableViewCellSelectionStyleNone;
}

- (void)updateFrame {
    CGRect frame = self.frame;
    if (!self.selected) {
        frame.size.height = 40;
    } else {
        frame.size.height = 150;
    }
    [self setFrame:frame];
}

-(void)setSelected:(BOOL)selected animated:(BOOL)animated{
    [super setSelected:selected animated:animated];
    _toolbar.hidden = !selected;
    if (!selected) {
        return;
    }
	if (player && [player isCreated]) {
		[player close];
	}

	player = [UILinphoneAudioPlayer audioPlayerWithFilePath:[self recording]];

    [player.view removeFromSuperview];
    [self addSubview:player.view];
    [self bringSubviewToFront:player.view];
    player.view.frame = _playerView.frame;
    player.view.bounds = _playerView.bounds;
    [player open];
}

- (void)onShareButtonPressed {
    UIActivityViewController *activityVC = [[UIActivityViewController alloc] initWithActivityItems:@[[NSURL fileURLWithPath:_recording]] applicationActivities:nil];
    [activityVC setCompletionWithItemsHandler:^(UIActivityType __nullable activityType, BOOL completed, NSArray * __nullable returnedItems, NSError * __nullable activityError) {
        //This is used to select the same row when we get back to the recordings view.
        NSString *file = player.file;
        //This reloads the view, if don't it's empty for some reason. Idealy we'd want to do this before closing the view but it's functionnal.
        [PhoneMainView.instance popCurrentView];
        [PhoneMainView.instance changeCurrentView:RecordingsListView.compositeViewDescription];
        [[(RecordingsListView *)VIEW(RecordingsListView) tableController] setSelected:file];
    }];
    [PhoneMainView.instance presentViewController:activityVC animated:YES completion:nil];
}

@end
