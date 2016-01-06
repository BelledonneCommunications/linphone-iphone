//
//  UILabel+Boldify.h
//  linphone
//
//  Created by guillaume on 20/05/2015.
//  Copyright (c) 2015 Urmet. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface UILabel (Boldify)

- (void)boldSubstring:(NSString *)substring;
- (void)boldRange:(NSRange)range;

@end
