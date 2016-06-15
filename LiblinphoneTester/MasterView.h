//
//  MasterViewController.h
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import <UIKit/UIKit.h>

@class DetailTableView;

@interface MasterView : UITableViewController

@property(strong, nonatomic) DetailTableView *detailViewController;

@end
