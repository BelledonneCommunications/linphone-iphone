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
@property (weak, nonatomic) NSString *file;
@property (weak, nonatomic) NSTimer *refreshTimer;

+ (id)audioPlayerWithFilePath:(NSString *)filePath;
- (void)close;
- (BOOL)isOpened;
- (BOOL)isCreated;
- (void)open;
- (void)pause;
- (void)setFile:(NSString *)fileName;
- (IBAction)onPlay:(id)sender;
- (IBAction)onStop:(id)sender;
- (IBAction)onTapTimeBar:(UITapGestureRecognizer *)sender;
@end
