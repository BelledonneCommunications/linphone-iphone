//
//  UACellBackgroundView.h
//  Ambiance
//
//  Created by Matt Coneybeare on 1/31/10.
//  Copyright 2010 Urban Apps LLC. All rights reserved.
//
//  Modified by Diorcet Yann on 07/12/12

#import <Foundation/Foundation.h>

#import <UIKit/UIKit.h>

typedef enum  {
    UACellBackgroundViewPositionSingle = 0,
    UACellBackgroundViewPositionTop, 
    UACellBackgroundViewPositionBottom,
    UACellBackgroundViewPositionMiddle
} UACellBackgroundViewPosition;

@interface UACellBackgroundView : UIView {
}

@property(nonatomic) UACellBackgroundViewPosition position;
@property(nonatomic, copy) UIColor *backgroundColor;
@property(nonatomic, copy) UIColor *borderColor;
@property(assign) BOOL automaticPositioning;

@end
