//
//  UIRoundedImageView.h
//  linphone
//
//  Created by guillaume on 13/05/2014.
//
//

#import <UIKit/UIKit.h>

@interface UIRoundedImageView : UIImageView

- (void)setImage:(UIImage *)image;
- (void)setImage:(UIImage *)image bordered:(BOOL)bordered withRoundedRadius:(BOOL)rounded;

- (void)setBordered:(BOOL)bordered;
- (void)setRoundRadius;

@end
