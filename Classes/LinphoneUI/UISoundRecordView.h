//
//  UISoundRecordView.h
//  linphone
//
//  Created by David Idmansour on 09/07/2018.
//

#import <UIKit/UIKit.h>

typedef enum LinphoneRecordState {
    LinphoneRecording,
    LinphoneRecordPaused,
    LinphoneRecordFinished,
    LinphoneRecordNotStarted
} LinphoneRecordState;

@interface UISoundRecordView : UIView
@property (strong, nonatomic) IBOutlet UIView *recordView;
@property (weak, nonatomic) IBOutlet UIButton *recordButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property (weak, nonatomic) IBOutlet UILabel *recordLength;
@property (weak, nonatomic) IBOutlet UIButton *cancelButton;
@property (strong, nonatomic) IBOutlet UIView *sendingView;
@property (weak, nonatomic) IBOutlet UIButton *sendButton;
@property (weak, nonatomic) IBOutlet UIView *playerView;

- (void)reset;
- (void)closePlayer;
- (IBAction)onRecord:(UIButton *)sender;
- (IBAction)onStop:(UIButton *)sender;
- (IBAction)onCancel:(UIButton *)sender;
- (IBAction)onSend:(UIButton *)sender;
@end
