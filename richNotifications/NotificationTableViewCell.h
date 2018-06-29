//
//  NotificationTableViewCell.h
//  richNotifications
//
//  Created by David Idmansour on 26/06/2018.
//

#import <UIKit/UIKit.h>

@interface NotificationTableViewCell : UITableViewCell
@property (weak, nonatomic) IBOutlet UIImageView *contactImage;
@property (weak, nonatomic) IBOutlet UILabel *nameDate;
@property (strong, nonatomic) IBOutlet UITextView *msgText;
@property (weak, nonatomic) IBOutlet UILabel *imdm;
@property (weak, nonatomic) IBOutlet UIImageView *background;
@property (weak, nonatomic) IBOutlet UIImageView *bottomBarColor;
@property (weak, nonatomic) IBOutlet UIImageView *imdmImage;
@property BOOL isOutgoing;
@property float width;
@property float height;

- (CGSize)ViewSizeForMessage:(NSString *)chat withWidth:(int)width;

@end
