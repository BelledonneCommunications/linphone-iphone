//
//  UIImageViewDeletable.h
//  linphone
//
//  Created by benjamin_verdier on 28/06/2018.
//

#import <UIKit/UIKit.h>

@protocol UIImageViewDeletableDelegate

@required

- (void)deleteImageWithAssetId:(NSString *)assetId;

@end

@interface UIImageViewDeletable : UICollectionViewCell

@property NSString *assetId;
@property(nonatomic, strong) id<UIImageViewDeletableDelegate> deleteDelegate;
@property (weak, nonatomic) IBOutlet UIImageView *image;

- (IBAction)onDeletePressed;

@end
