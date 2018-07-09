//
//  UISoundRecordView.h
//  linphone
//
//  Created by David Idmansour on 09/07/2018.
//

#import <UIKit/UIKit.h>

@interface UISoundRecordView : UIView
@property (strong, nonatomic) IBOutlet UIView *recordView;
@property (weak, nonatomic) IBOutlet UIButton *recordButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property (weak, nonatomic) IBOutlet UILabel *recordLength;
@property (weak, nonatomic) IBOutlet UIButton *cancelButton;

- (IBAction)onRecord:(UIButton *)sender;
- (IBAction)onStop:(UIButton *)sender;
- (IBAction)onCancel:(UIButton *)sender;
@end
