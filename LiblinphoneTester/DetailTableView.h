//
//  DetailViewController.h
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(int, TestState) { TestStateIdle, TestStatePassed, TestStateInProgress, TestStateFailed };

@interface TestItem : NSObject

@property(strong, nonatomic) NSString *suite;
@property(strong, nonatomic) NSString *name;
@property(nonatomic) TestState state;

- (id)initWithName:(NSString *)name fromSuite:(NSString *)suite;
+ (TestItem *)testWithName:(NSString *)name fromSuite:(NSString *)suite;

@end

@interface DetailTableView : UITableViewController

@property(strong, nonatomic) NSString *detailItem;

@end
