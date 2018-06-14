//
//  TodayViewController.h
//  latestCallsWidget
//
//  Created by David Idmansour on 06/06/2018.
//

#import <UIKit/UIKit.h>

@interface TodayViewController : UIViewController
@property (strong, nonatomic) IBOutletCollection(UIStackView) NSArray *stackViews;
@property (strong, nonatomic) NSMutableArray *contactsToDisplay;
@property (strong, nonatomic) NSMutableDictionary *logs;
@property (strong, nonatomic) NSMutableDictionary *imgs;
@property (strong, nonatomic) NSMutableArray *sortedDates;
@property (strong, nonatomic) NSMutableArray *logIds;
@property (strong) dispatch_semaphore_t sem;

- (IBAction)firstButtonTapped;
- (IBAction)secondButtonTapped;
- (IBAction)thirdButtonTapped;
- (IBAction)fourthButtonTapped;
@end
