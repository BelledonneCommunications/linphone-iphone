//
//  UILinphoneAudioPlayer.h
//  linphone
//
//  Created by David Idmansour on 13/07/2018.
//

#import <UIKit/UIKit.h>

@interface UILinphoneAudioPlayer : UIViewController
@property (weak, nonatomic) IBOutlet UIButton *playButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property (weak, nonatomic) IBOutlet UILabel *timeLabel;
@property (weak, nonatomic) IBOutlet UIProgressView *timeProgress;

+ (id)audioPlayerWithFilePath:(NSString *)filePath;
+ (void)registerPlayer:(UILinphoneAudioPlayer *)aPlayer forMessage:(LinphoneChatMessage *)msg;
+ (UILinphoneAudioPlayer *)playerForMessage:(LinphoneChatMessage *)msg;
+ (void)closePlayers;
- (void)close;
- (BOOL)isOpened;
- (void)open;

- (IBAction)onPlay:(id)sender;
- (IBAction)onStop:(id)sender;
- (IBAction)onTapTimeBar:(UITapGestureRecognizer *)sender;
@end
