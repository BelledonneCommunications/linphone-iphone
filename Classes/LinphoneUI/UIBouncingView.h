//
//  UIBouncingView.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/12/15.
//
//

#import <UIKit/UIKit.h>

@interface UIBouncingView : UIView

- (void)startAnimating:(BOOL)animated;
- (void)stopAnimating:(BOOL)animated;

@end
