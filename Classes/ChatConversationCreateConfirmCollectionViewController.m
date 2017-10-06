//
//  ChatConversationCreateConfirmCollectionViewController.m
//  linphone
//
//  Created by REIS Benjamin on 05/10/2017.
//

#import "ChatConversationCreateConfirmCollectionViewController.h"
#import "UIChatCreateConfirmCollectionViewCell.h"

@interface ChatConversationCreateConfirmCollectionViewController ()

@end

@implementation ChatConversationCreateConfirmCollectionViewController

static NSString * const reuseIdentifier = @"Cell";

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Register cell classes
    [self.collectionView registerClass:[UIChatCreateConfirmCollectionViewCell class] forCellWithReuseIdentifier:reuseIdentifier];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

@end
