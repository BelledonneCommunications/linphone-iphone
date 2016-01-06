//
//  ChatConversationSearchTableView.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/09/15.
//
//

#import <UIKit/UIKit.h>

@interface ChatConversationCreateTableView : UITableViewController <UISearchBarDelegate>
@property(weak, nonatomic) IBOutlet UISearchBar *searchBar;

@end
