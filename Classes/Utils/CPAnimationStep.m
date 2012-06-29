
//  Created by Yang Meyer on 26.07.11.
//  Copyright 2011-2012 compeople AG. All rights reserved.

#import "CPAnimationStep.h"

@interface CPAnimationStep()
/** A temporary reverse queue of animation steps, i.e. from last to first.
 It is created when the step is run, and is modified during the animation, 
 and is destroyed when the animation finishes. */
@property (nonatomic, strong) NSMutableArray* consumableSteps;
@end

@implementation CPAnimationStep

@synthesize delay, duration, step, options;
@synthesize consumableSteps;

#pragma mark overrides


#pragma mark construction

+ (id) after:(NSTimeInterval)delay animate:(AnimationStep)step {
	return [self after:delay for:0.0 options:0 animate:step];
}

+ (id) for:(NSTimeInterval)duration animate:(AnimationStep)step {
   return [self after:0.0 for:duration options:0 animate:step];
}

+ (id) after:(NSTimeInterval)delay for:(NSTimeInterval)duration animate:(AnimationStep)step {
	return [self after:delay for:duration options:0 animate:step];
}

+ (id) after:(NSTimeInterval)theDelay
		 for:(NSTimeInterval)theDuration
	 options:(UIViewAnimationOptions)theOptions
	 animate:(AnimationStep)theStep {
	
	CPAnimationStep* instance = [[self alloc] init];
	if (instance) {
		instance.delay = theDelay;
		instance.duration = theDuration;
		instance.options = theOptions;
		instance.step = [theStep copy];
	}
	return instance;
}

#pragma mark action

// From http://stackoverflow.com/questions/4007023/blocks-instead-of-performselectorwithobjectafterdelay
+ (void) runBlock:(AnimationStep)block afterDelay:(NSTimeInterval)delay {
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC*delay), dispatch_get_current_queue(), block);
}

- (NSArray*) animationStepArray {
	// subclasses must override this!
	return [NSArray arrayWithObject:self];
}

- (void) runAnimated:(BOOL)animated {
	if (!self.consumableSteps) {
		self.consumableSteps = [[NSMutableArray alloc] initWithArray:[self animationStepArray]];
	}
	if (![self.consumableSteps count]) { // recursion anchor
		self.consumableSteps = nil;
		return; // we're done
	}
	AnimationStep completionStep = ^{
		[self.consumableSteps removeLastObject];
		[self runAnimated:animated]; // recurse!
	};
	CPAnimationStep* currentStep = [self.consumableSteps lastObject];
	// Note: do not animate to short steps
	if (animated && currentStep.duration >= 0.02) {
		[UIView animateWithDuration:currentStep.duration
							  delay:currentStep.delay
							options:currentStep.options
						 animations:currentStep.step
						 completion:^(BOOL finished) {
							 if (finished) {
								 completionStep();
							 }
						 }];
	} else {
		void (^execution)(void) = ^{
			currentStep.step();
			completionStep();
		};
		
		if (animated && currentStep.delay) {
			[CPAnimationStep runBlock:execution afterDelay:currentStep.delay];
		} else {
			execution();
		}
	}
}

- (void) run {
	[self runAnimated:YES];
}

#pragma mark - pretty-print

- (NSString*) description {
	NSMutableString* result = [[NSMutableString alloc] initWithCapacity:100];
	[result appendString:@"\n["];
	if (self.delay > 0.0) {
		[result appendFormat:@"after:%.1f ", self.delay];
	}
	if (self.duration > 0.0) {
		[result appendFormat:@"for:%.1f ", self.duration];
	}
	if (self.options > 0) {
		[result appendFormat:@"options:%d ", self.options];
	}
	[result appendFormat:@"animate:%@", self.step];
	[result appendString:@"]"];
	return result;
}

@end
