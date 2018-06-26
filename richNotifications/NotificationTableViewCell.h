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
@property (weak, nonatomic) IBOutlet UITextView *msgText;

@end
