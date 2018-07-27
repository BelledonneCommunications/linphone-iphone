//
//  UIRecordingCell.h
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import <UIKit/UIKit.h>

@interface UIRecordingCell : UITableViewCell

@property (weak, nonatomic) IBOutlet UIView *playerView;
@property (weak, nonatomic) IBOutlet UILabel *nameLabel;
@property (strong, nonatomic) IBOutlet UIToolbar *toolbar;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *shareButton;


@property(nonatomic, assign) __block NSString *recording;

- (id)initWithIdentifier:(NSString*)identifier;

- (void)updateFrame;

@end
