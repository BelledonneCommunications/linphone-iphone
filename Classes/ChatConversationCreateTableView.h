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
@property(nonatomic) Boolean allFilter;
@property(nonatomic) Boolean notFirstTime;
@property(nonatomic, strong) NSMutableArray *contactsGroup;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (weak, nonatomic) IBOutlet UIIconButton *controllerNextButton;

@property(nonatomic) Boolean isForEditing;
- (void) loadData;

@end
