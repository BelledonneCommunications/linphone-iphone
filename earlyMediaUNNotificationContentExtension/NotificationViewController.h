//
//  NotificationViewController.h
//  earlyMediaUNNotificationContentExtension
//
//  Created by Tof on 29/09/2019.
//

#import <UIKit/UIKit.h>
#include "linphone/linphonecore.h"
#define SHARED_GROUP_NAME @"group.org.linphone.phone.earlymediaExtension"

@interface NotificationViewController : UIViewController
@property (weak, nonatomic) IBOutlet UIView *videoPreview;
@property (weak, nonatomic) IBOutlet UILabel *uri;
@property (weak, nonatomic) IBOutlet UILabel *state;
@property (weak, nonatomic) IBOutlet UIView *mediaPlayPauseButton;

@end
