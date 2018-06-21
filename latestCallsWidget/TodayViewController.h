//
//  TodayViewController.h
//  latestCallsWidget
//
//  Created by David Idmansour on 06/06/2018.
//

#import <UIKit/UIKit.h>

@interface TodayViewController : UIViewController
@property (strong, nonatomic) IBOutletCollection(UIStackView) NSArray *stackViews;
@property (strong, nonatomic) NSMutableArray *imgs;
@property (strong, nonatomic) NSMutableArray *logIds;
@property (strong, nonatomic) NSMutableArray *displayNames;

- (IBAction)firstButtonTapped;
- (IBAction)secondButtonTapped;
- (IBAction)thirdButtonTapped;
- (IBAction)fourthButtonTapped;
@end
