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
@property(nonatomic, strong) NSMutableArray *contactsGroup;
@property(nonatomic, strong) NSMutableDictionary *contactsDict;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (weak, nonatomic) IBOutlet UIIconButton *controllerNextButton;

- (void) loadData;

@end
