//
//  SideMenuTableViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import <UIKit/UIKit.h>

// the block to execute when an entry tapped
typedef void (^SideMenuEntryBlock)(void);

@interface SideMenuEntry : NSObject {
  @public
	NSString *title;
	SideMenuEntryBlock onTapBlock;
};
@end

@interface SideMenuTableView : UITableViewController

@property(nonatomic, retain) NSMutableArray *sideMenuEntries;

@end
