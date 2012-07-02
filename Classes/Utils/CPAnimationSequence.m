
//  Created by Yang Meyer on 26.07.11.
//  Copyright 2011-2012 compeople AG. All rights reserved.

#import "CPAnimationSequence.h"

@interface CPAnimationStep(hidden)
- (NSArray*) animationStepArray;
@end

@interface CPAnimationSequence()
@property (nonatomic, strong, readwrite) NSArray* steps;
@end

#pragma mark -
@implementation CPAnimationSequence

@synthesize steps;

#pragma mark - Object lifecycle

+ (id) sequenceWithSteps:(CPAnimationStep*)first, ... {
	CPAnimationSequence* instance = [[self alloc] init];
	if (instance) {
		NSMutableArray* tempSteps = [[NSMutableArray alloc] initWithCapacity:10];
		va_list args;
		va_start(args, first);
		[tempSteps insertObject:first atIndex:0];
		CPAnimationStep* aStep;
		while ((aStep = va_arg(args, CPAnimationStep*))) {
			[tempSteps insertObject:aStep atIndex:0];
		}
		instance.steps = [NSArray arrayWithArray:tempSteps];
		va_end(args);
        [tempSteps release];
	}
	return instance;
}

- (void) dealloc {
    [steps release];
    [super dealloc];
}

#pragma mark - property override

- (void) setDelay:(NSTimeInterval)delay {
    NSAssert(NO, @"Setting a delay on a sequence is undefined and therefore disallowed!");
}

- (void) setDuration:(NSTimeInterval)duration {
    NSAssert(NO, @"Setting a duration on a sequence is undefined and therefore disallowed!");
}

- (void) setOptions:(UIViewAnimationOptions)options {
    NSAssert(NO, @"Setting options on a sequence is undefined and therefore disallowed!");
}

#pragma mark - build the sequence

- (NSArray*) animationStepArray {
	NSMutableArray* array = [NSMutableArray arrayWithCapacity:[self.steps count]];
	for (CPAnimationStep* current in self.steps) {
		[array addObjectsFromArray:[current animationStepArray]];
	}
	return array;
}

#pragma mark - pretty-print

- (NSString*) description {
	NSMutableString* sequenceBody = [[NSMutableString alloc] initWithCapacity:100*[self.steps count]];
	for (CPAnimationStep* step in self.steps) {
		[sequenceBody appendString:[step description]];
	}
	// indent
	[sequenceBody replaceOccurrencesOfString:@"\n"
								 withString:@"\n  "
									options:NSCaseInsensitiveSearch
									  range:NSMakeRange(0, [sequenceBody length])];
	return [NSString stringWithFormat:@"\n(sequence:%@\n)", sequenceBody];
}

@end
