//
//  ChatConversationSearchTableView.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/09/15.
//
//

#import <UIKit/UIKit.h>

@interface ChatConversationCreateTableView : UITableViewController <UISearchBarDelegate>
@property(nonatomic) Boolean allFilter;
@property(nonatomic) Boolean notFirstTime;
@property(nonatomic, strong) NSMutableArray *contactsGroup;
@property(nonatomic) LinphoneMagicSearch *magicSearch;

@property(weak, nonatomic) IBOutlet UISearchBar *searchBar;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (weak, nonatomic) IBOutlet UIIconButton *controllerNextButton;
@property (weak, nonatomic) IBOutlet UIView *waitView;

@property(nonatomic) Boolean isForEditing;
@property(nonatomic) Boolean isGroupChat;
@property(nonatomic) Boolean isEncrypted;
- (void) loadData;

@end
