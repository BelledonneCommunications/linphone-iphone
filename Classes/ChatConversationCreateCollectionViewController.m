//
//  ChatConversationCreateCollectionViewController.m
//  linphone
//
//  Created by REIS Benjamin on 03/10/2017.
//

#import "ChatConversationCreateCollectionViewController.h"

@interface ChatConversationCreateCollectionViewController ()

@end

@implementation ChatConversationCreateCollectionViewController

static NSString * const reuseIdentifier = @"Cell";

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Register cell classes
    [self.collectionView registerClass:[UICollectionViewCell class] forCellWithReuseIdentifier:reuseIdentifier];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

@end
