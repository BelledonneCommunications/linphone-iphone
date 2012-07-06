
//  Created by Yang Meyer on 26.07.11.
//  Copyright 2011-2012 compeople AG. All rights reserved.

#import <Foundation/Foundation.h>
#import "CPAnimationStep.h"

/** 
 A CPAnimationSequence defines a sequence of CPAnimationStep objects, which can
 be `-run` animatedly or non-animatedly.
 
 CPAnimationSequence implements the Composite design pattern, with CPAnimationStep
 as the base class.
 */
@interface CPAnimationSequence : CPAnimationStep

#pragma mark - constructors

+ (id) sequenceWithSteps:(CPAnimationStep*)first, ... NS_REQUIRES_NIL_TERMINATION;

#pragma mark - properties

/** Animations steps, from first to last. */
@property (nonatomic, strong, readonly) NSArray* steps;

@end
