//
//  MasterViewController.h
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import <UIKit/UIKit.h>

void LSLog(NSString* fmt, ...);

NSMutableArray* lastLogs;
NSString* const  kLogsUpdateNotification;

@class DetailViewController;

@interface MasterViewController : UITableViewController

@property (strong, nonatomic) DetailViewController *detailViewController;

@end

