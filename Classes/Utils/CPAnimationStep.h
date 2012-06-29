
//  Created by Yang Meyer on 26.07.11.
//  Copyright 2011 compeople AG. All rights reserved.

#import <Foundation/Foundation.h>

/** Helper type definition for the component. */
typedef void (^AnimationStep)(void);

/** 
 A CPAnimationStep defines a single animation object with a delay, duration, execution block and animation options.
 */
@interface CPAnimationStep : NSObject

#pragma mark - constructors

+ (id) after:(NSTimeInterval)delay
	 animate:(AnimationStep)step;

+ (id) for:(NSTimeInterval)duration
   animate:(AnimationStep)step;

+ (id) after:(NSTimeInterval)delay
		 for:(NSTimeInterval)duration
	 animate:(AnimationStep)step;

+ (id) after:(NSTimeInterval)delay
		 for:(NSTimeInterval)duration
	 options:(UIViewAnimationOptions)theOptions
	 animate:(AnimationStep)step;

#pragma mark - properties (normally already set by the constructor)

@property (nonatomic) NSTimeInterval delay;
@property (nonatomic) NSTimeInterval duration;
@property (nonatomic, copy) AnimationStep step;
@property (nonatomic) UIViewAnimationOptions options;

#pragma mark - execution

/** Starts the step execution. */
- (void) runAnimated:(BOOL)animated;
/** Shortcut for [step runAnimated:YES] */
- (void) run;

@end
