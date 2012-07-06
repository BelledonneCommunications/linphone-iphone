//
//  TUDebugLoggingAssistant.h
//  NinePatch
//
//  Copyright 2010 Tortuga 22, Inc. All rights reserved.
//

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>


@interface TUDebugLoggingAssistant : NSObject {
	NSString *_sessionIdentifier;
	NSMutableSet *_activatedLoggingTags;
}

// Synthesized Properties
@property(nonatomic, retain, readonly) NSString *sessionIdentifier;
@property(nonatomic, retain, readonly) NSMutableSet *activatedLoggingTags;

// Init + Dealloc
-(id)init;
-(void)dealloc;

// Managing Logging
-(void)startLoggingTag:(NSString *)loggingTag;
-(void)stopLoggingTag:(NSString *)loggingTag;
-(void)startLoggingTags:(NSString *)loggingTag,...;
-(void)stopLoggingTags:(NSString *)loggingTag,...;
-(void)startLoggingTagsFromArray:(NSArray *)loggingTags;
-(void)stopLoggingTagsFromArray:(NSArray *)loggingTags;

// Formatting
-(NSString *)formattedImageLogFilenameForTimestamp:(NSTimeInterval)timestamp specifiedFileName:(NSString *)specifiedFileName;

// Log-Checking
-(BOOL)shouldLogLineWithTag:(NSString *)tag;

// Singleton Access
+(id)shared;

// Managing Logging
+(void)startLoggingTag:(NSString *)loggingTag;
+(void)stopLoggingTag:(NSString *)loggingTag;
+(void)startLoggingTags:(NSString *)loggingTag,...;
+(void)stopLoggingTags:(NSString *)loggingTag,...;
+(void)startLoggingTagsFromArray:(NSArray *)loggingTags;
+(void)stopLoggingTagsFromArray:(NSArray *)loggingTags;

// Formatting
+(NSString *)formattedImageLogFilenameForTimestamp:(NSTimeInterval)timestamp specifiedFileName:(NSString *)specifiedFileName;

// Log-Checking
+(BOOL)shouldLogLineWithTag:(NSString *)tag;

@end
