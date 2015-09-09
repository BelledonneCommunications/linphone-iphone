//
//  HistoryDetailsTableViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 27/07/15.
//
//

#import <UIKit/UIKit.h>

@interface HistoryDetailsTableView : UITableViewController {
  @private
	NSMutableArray *callLogs;
}
- (void)loadData;
@end
