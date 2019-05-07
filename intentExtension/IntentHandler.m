//
//  IntentHandler.m
//
//  Created by Christophe Deschamps on 06/05/2019.
//

#import "IntentHandler.h"


@interface IntentHandler () <INSendMessageIntentHandling>

@end

@implementation IntentHandler

- (id)handlerForIntent:(INIntent *)intent {
	return self;
}

- (void)handleSendMessage:(INSendMessageIntent *)intent completion:(void (^)(INSendMessageIntentResponse *response))completion {
	NSUserActivity *userActivity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass([INSendMessageIntent class])];
	INSendMessageIntentResponse *response = [[INSendMessageIntentResponse alloc] initWithCode:INSendMessageIntentResponseCodeSuccess userActivity:userActivity];
	NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:[NSString stringWithFormat:@"group.%@",[[NSBundle mainBundle] bundleIdentifier]]];
	[defaults setObject:intent.content forKey:@"rejectionmessage"];
	[defaults synchronize];
	completion(response);
}


@end

