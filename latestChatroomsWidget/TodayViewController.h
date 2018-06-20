//
//  TodayViewController.h
//  latestChatroomsWidget
//
//  Created by David Idmansour on 18/06/2018.
//

#import <UIKit/UIKit.h>

@interface TodayViewController : UIViewController
@property (strong, nonatomic) IBOutletCollection(UIStackView) NSArray *stackViews;
@property (strong, nonatomic) NSMutableArray *addresses;
@property (strong, nonatomic) NSMutableArray *localAddress;
@property (strong, nonatomic) NSMutableArray *displayNames;
@property (strong, nonatomic) NSMutableArray *isConf;
@property (strong, nonatomic) NSMutableArray *imgs;

- (IBAction)firstButtonTapped;
- (IBAction)secondButtonTapped;
- (IBAction)thirdButtonTapped;
- (IBAction)fourthButtonTapped;
@end
