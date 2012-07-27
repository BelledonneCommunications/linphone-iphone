//
//  TUDebugLoggingAssistant.m
//  NinePatch
//
//  Copyright 2010 Tortuga 22, Inc. All rights reserved.
//

#import "TUDebugLoggingAssistant.h"

@interface TUDebugLoggingAssistant ()

@property(nonatomic, retain, readwrite) NSString *sessionIdentifier;
@property(nonatomic, retain, readwrite) NSMutableSet *activatedLoggingTags;

@end


@implementation TUDebugLoggingAssistant

#pragma mark Synthesized Properties
@synthesize sessionIdentifier = _sessionIdentifier;
@synthesize activatedLoggingTags = _activatedLoggingTags;

#pragma mark Init + Dealloc
-(id)init {
	if (self = [super init]) {
		self.activatedLoggingTags = [NSMutableSet set];
		NSString *uuidString = @"UUID_ERROR_ENCOUNTERED";
		@try {
			CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
			if (uuid) {
				@try {
					uuidString = ((NSString *) CFUUIDCreateString(kCFAllocatorDefault, uuid));
				}
				@catch (NSException * e) {
					NPLogException(e);
					if (uuidString) {
						@try {
							CFRelease(uuidString);
						}
						@catch (NSException * ee) {
							NPLogException(ee);
						}
						@finally {
							uuidString = @"UUID_ERROR_ENCOUNTERED";
						}
					}
				}
				@finally {
					@try {
						CFRelease(uuid);
					}
					@catch (NSException * e) {
						NPLogException(e);
					}
				}
			}
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
		@finally {
			if (uuidString) {
				self.sessionIdentifier = uuidString;
				[self.sessionIdentifier release]; // drops retain count back to 1
			}			
		}
	}
	return self;
}

#pragma mark -
-(void)dealloc {
	self.sessionIdentifier = nil;
	self.activatedLoggingTags = nil;
	[super dealloc];
}

#pragma mark Managing Logging
-(void)startLoggingTag:(NSString *)loggingTag {
	NPAOInputLog(loggingTag);
	NPParameterAssertNotNilIsKindOfClass(loggingTag,NSString);
	NPAssertPropertyNonNil(activatedLoggingTags);
	if (loggingTag) {
		@try {
			[self.activatedLoggingTags addObject:loggingTag];
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
	}
}

-(void)stopLoggingTag:(NSString *)loggingTag {
	NPAOInputLog(loggingTag);
	NPParameterAssertNotNilIsKindOfClass(loggingTag,NSString);
	NPAssertPropertyNonNil(activatedLoggingTags);
	if (loggingTag) {
		@try {
			[self.activatedLoggingTags removeObject:loggingTag];
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
	}
}

-(void)startLoggingTags:(NSString *)loggingTag,... {
	id eachObject;
	va_list argumentList;
	if (loggingTag) {
		NSMutableArray *workingArray = [NSMutableArray arrayWithObject:loggingTag];
		va_start(argumentList, loggingTag);
		@try {
			while ((eachObject = va_arg(argumentList, id))) {
				@try {
					[workingArray addObject:eachObject];
				}
				@catch (NSException * e) {
					NPLogException(e);
				}
			}
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
		@finally {
			@try {
				va_end(argumentList);
			}
			@catch (NSException * e) {
				NPLogException(e);
			}
			@finally {
				if (workingArray) {
					[self startLoggingTagsFromArray:workingArray];
				}
			}
		}
	}
}

-(void)stopLoggingTags:(NSString *)loggingTag,... {
	id eachObject;
	va_list argumentList;
	if (loggingTag) {
		NSMutableArray *workingArray = [NSMutableArray arrayWithObject:loggingTag];
		va_start(argumentList, loggingTag);
		@try {
			while ((eachObject = va_arg(argumentList, id))) {
				@try {
					[workingArray addObject:eachObject];
				}
				@catch (NSException * e) {
					NPLogException(e);
				}
			}
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
		@finally {
			@try {
				va_end(argumentList);
			}
			@catch (NSException * e) {
				NPLogException(e);
			}
			@finally {
				if (workingArray) {
					[self stopLoggingTagsFromArray:workingArray];
				}
			}
		}
	}
}

-(void)startLoggingTagsFromArray:(NSArray *)loggingTags {
	NPAOInputLog(loggingTags);
	NPParameterAssertNotNilIsKindOfClass(loggingTags,NSArray);
	NPAssertPropertyNonNil(activatedLoggingTags);
	if (loggingTags) {
		// N.B.: we're not shooting for speed here
		// b/c if you're even instantiating an instance of this
		// class it means you're in a debug frenzy and have other
		// issues.
		//
		// So here we break out the for loop and insert objects
		// individually so that the asserts in the [self startLoggingTag:tag]
		// will get noticed if you're smuggling non-string logging tags in
		// by passing them in as part of an array.
		//
		// No point having type-induced errors in your debugging assistant. 
		for (NSString *loggingTag in loggingTags) {
			[self startLoggingTag:loggingTag];
		}
	}	
}

-(void)stopLoggingTagsFromArray:(NSArray *)loggingTags {
	NPAOInputLog(loggingTags);
	NPParameterAssertNotNilIsKindOfClass(loggingTags,NSArray);
	NPAssertPropertyNonNil(activatedLoggingTags);
	if (loggingTags) {
		// CF comment in startLoggingTagsFromArray
		// for some context here
		for (NSString *loggingTag in loggingTags) {
			[self stopLoggingTag:loggingTag];
		}
	}		
}

#pragma mark Formatting
-(NSString *)formattedImageLogFilenameForTimestamp:(NSTimeInterval)timestamp specifiedFileName:(NSString *)specifiedFileName {
	NPAInputLog(@"formattedImageLogFilenameForTimestamp:'%f' specifiedFileName:'%@'",timestamp,specifiedFileName);
	NPParameterAssertNotNilIsKindOfClass(specifiedFileName,NSString);
	NPAssertPropertyNonNil(sessionIdentifier);
	NSString *outString = nil;
	@try {
		if (specifiedFileName) {
			outString = [NSString stringWithFormat:@"%@_%@_%.0f.png",self.sessionIdentifier,specifiedFileName,timestamp];
		} else {
			outString = [NSString stringWithFormat:@"%@_%.0f.png",self.sessionIdentifier,timestamp];
		}
	}
	@catch (NSException * e) {
		NPLogException(e);
	}
	NPOOutputLog(outString);
	return outString;
}

#pragma mark Log-Checking
-(BOOL)shouldLogLineWithTag:(NSString *)tag {
	NPAInputLog(@"shouldLogLineWithTag:'%@'",tag);
	NPParameterAssertNotNilIsKindOfClass(tag,NSString);
	NPAssertPropertyNonNil(activatedLoggingTags);
	BOOL shouldLogLineWithTag = NO;
	if (tag) {
		@try {
			shouldLogLineWithTag = [self.activatedLoggingTags containsObject:tag];
		}
		@catch (NSException * e) {
			NPLogException(e);
			shouldLogLineWithTag = NO;
		}
	}
	NPBOutputLog(shouldLogLineWithTag);
	return shouldLogLineWithTag;
}

#pragma mark Singleton Access
+(id)shared {
	static id shared;
	if (!shared) {
		shared = [[[self class] alloc] init];
	}
	NPAssertNilOrIsKindOfClass(shared,TUDebugLoggingAssistant);
	return shared;
}

#pragma mark Managing Logging
+(void)startLoggingTag:(NSString *)loggingTag {
	[[self shared] startLoggingTag:loggingTag];
}

+(void)stopLoggingTag:(NSString *)loggingTag {
	[[self shared] stopLoggingTag:loggingTag];
}

#pragma mark -
+(void)startLoggingTags:(NSString *)loggingTag,... {
	id eachObject;
	va_list argumentList;
	if (loggingTag) {
		NSMutableArray *workingArray = [NSMutableArray arrayWithObject:loggingTag];
		va_start(argumentList, loggingTag);
		@try {
			while ((eachObject = va_arg(argumentList, id))) {
				@try {
					[workingArray addObject:eachObject];
				}
				@catch (NSException * e) {
					NPLogException(e);
				}
			}
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
		@finally {
			@try {
				va_end(argumentList);
			}
			@catch (NSException * e) {
				NPLogException(e);
			}
			@finally {
				if (workingArray) {
					[self startLoggingTagsFromArray:workingArray];
				}
			}
		}
	}
}

+(void)stopLoggingTags:(NSString *)loggingTag,... {
	id eachObject;
	va_list argumentList;
	if (loggingTag) {
		NSMutableArray *workingArray = [NSMutableArray arrayWithObject:loggingTag];
		va_start(argumentList, loggingTag);
		@try {
			while ((eachObject = va_arg(argumentList, id))) {
				@try {
					[workingArray addObject:eachObject];
				}
				@catch (NSException * e) {
					NPLogException(e);
				}
			}
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
		@finally {
			@try {
				va_end(argumentList);
			}
			@catch (NSException * e) {
				NPLogException(e);
			}
			@finally {
				if (workingArray) {
					[self stopLoggingTagsFromArray:workingArray];
				}
			}
		}
	}
}

#pragma mark -
+(void)startLoggingTagsFromArray:(NSArray *)loggingTags {
	[[self shared] startLoggingTagsFromArray:loggingTags];
}

+(void)stopLoggingTagsFromArray:(NSArray *)loggingTags {
	[[self shared] stopLoggingTagsFromArray:loggingTags];
}

#pragma mark Formatting
+(NSString *)formattedImageLogFilenameForTimestamp:(NSTimeInterval)timestamp specifiedFileName:(NSString *)specifiedFileName {
	return [[self shared] formattedImageLogFilenameForTimestamp:timestamp 
											  specifiedFileName:specifiedFileName];
}

#pragma mark Log-Checking
+(BOOL)shouldLogLineWithTag:(NSString *)tag {
	return [[self shared] shouldLogLineWithTag:tag];
}

@end
