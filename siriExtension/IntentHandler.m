//
//  IntentHandler.m
//  siriExtension
//
//  Created by David Idmansour on 18/07/2018.
//

#import "IntentHandler.h"

// As an example, this class is set up to handle Message intents.
// You will want to replace this or add other intents as appropriate.
// The intents you wish to handle must be declared in the extension's Info.plist.

// You can test your example integration by saying things to Siri like:
// "Send a message using <myApp>"
// "<myApp> John saying hello"
// "Search for messages in <myApp>"

@interface IntentHandler () <INSendMessageIntentHandling, INStartAudioCallIntentHandling>

@end

@implementation IntentHandler

- (id)handlerForIntent:(INIntent *)intent {
    // This is the default implementation.  If you want different objects to handle different intents,
    // you can override this and return the handler you want for that particular intent.
    
    return self;
}

#pragma mark - INSendMessageIntentHandling

// Implement resolution methods to provide additional information about your intent (optional).
- (void)resolveRecipientsForSendMessage:(INSendMessageIntent *)intent withCompletion:(void (^)(NSArray<INPersonResolutionResult *> *resolutionResults))completion {
    NSArray<INPerson *> *recipients = intent.recipients;
    // If no recipients were provided we'll need to prompt for a value.
    if (recipients.count == 0) {
        completion(@[[INPersonResolutionResult needsValue]]);
        return;
    }
    NSMutableArray<INPersonResolutionResult *> *resolutionResults = [NSMutableArray array];
    
    for (INPerson *recipient in recipients) {
        NSArray<INPerson *> *matchingContacts = @[recipient]; // Implement your contact matching logic here to create an array of matching contacts
        if (matchingContacts.count > 1) {
            // We need Siri's help to ask user to pick one from the matches.
            [resolutionResults addObject:[INPersonResolutionResult disambiguationWithPeopleToDisambiguate:matchingContacts]];

        } else if (matchingContacts.count == 1) {
            // We have exactly one matching contact
            [resolutionResults addObject:[INPersonResolutionResult successWithResolvedPerson:recipient]];
        } else {
            // We have no contacts matching the description provided
            [resolutionResults addObject:[INPersonResolutionResult unsupported]];
        }
    }
    completion(resolutionResults);
}

- (void)resolveContentForSendMessage:(INSendMessageIntent *)intent withCompletion:(void (^)(INStringResolutionResult *resolutionResult))completion {
    NSString *text = intent.content;
    if (text && ![text isEqualToString:@""]) {
        completion([INStringResolutionResult successWithResolvedString:text]);
    } else {
        completion([INStringResolutionResult needsValue]);
    }
}

// Once resolution is completed, perform validation on the intent and provide confirmation (optional).

- (void)confirmSendMessage:(INSendMessageIntent *)intent completion:(void (^)(INSendMessageIntentResponse *response))completion {
    // Verify user is authenticated and your app is ready to send a message.
    
    NSUserActivity *userActivity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass([INSendMessageIntent class])];
    INSendMessageIntentResponse *response = [[INSendMessageIntentResponse alloc] initWithCode:INSendMessageIntentResponseCodeReady userActivity:userActivity];
    completion(response);
}

// Handle the completed intent (required).

- (void)handleSendMessage:(INSendMessageIntent *)intent completion:(void (^)(INSendMessageIntentResponse *response))completion {
    // Implement your application logic to send a message here.
    
    NSUserActivity *userActivity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass([INSendMessageIntent class])];
    INSendMessageIntentResponse *response = [[INSendMessageIntentResponse alloc] initWithCode:INSendMessageIntentResponseCodeSuccess userActivity:userActivity];
    completion(response);
}

#pragma mark - INStartAudioCallIntentHandling

- (void)resolveContactsForStartAudioCall:(INStartAudioCallIntent *)intent withCompletion:(void (^)(NSArray<INPersonResolutionResult *> * _Nonnull))completion {
    NSArray<INPerson *> *contacts = intent.contacts;
    NSMutableArray<INPersonResolutionResult *> *responses = [NSMutableArray array];
    for (id iterator in contacts)
        [responses addObject:[INPersonResolutionResult unsupported]];
    if (contacts.count == 0) {
        completion(@[[INPersonResolutionResult needsValue]]);
        return;
    }
    NSString *contactName = [contacts.firstObject.displayName lowercaseString];
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.belledonne-communications.linphone.siri"];
    NSDictionary<NSString *, NSArray *> *addresses = [defaults objectForKey:@"addresses"];
    NSMutableArray<INPerson *> *matchingContacts = [NSMutableArray array];
    for (NSString *name in addresses.allKeys) {
        if ([[name lowercaseString] containsString:[contactName lowercaseString]] || [[contactName lowercaseString] containsString:[name lowercaseString]]) {
            INPersonHandle *handle = [[INPersonHandle alloc] initWithValue:[addresses objectForKey:name].firstObject type:INPersonHandleTypeUnknown];
            INPerson *contact = [[INPerson alloc] initWithPersonHandle:handle nameComponents:nil displayName:name image:nil contactIdentifier:nil customIdentifier:handle.value];
            [matchingContacts addObject:contact];
        }
    }
    INPersonResolutionResult *response;
    switch(matchingContacts.count) {
        case 0:
            response = [INPersonResolutionResult unsupported];
            break;
        case 1:
            response = [INPersonResolutionResult confirmationRequiredWithPersonToConfirm:matchingContacts.firstObject];
            break;
        default:
            response = [INPersonResolutionResult disambiguationWithPeopleToDisambiguate:matchingContacts];
    }
    [responses setObject:response atIndexedSubscript:0];
    completion(responses);
}

- (void)confirmStartAudioCall:(INStartAudioCallIntent *)intent completion:(void (^)(INStartAudioCallIntentResponse * _Nonnull))completion {
    NSUserActivity *activity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass(INStartAudioCallIntent.class)];
    INStartAudioCallIntentResponse *response = [[INStartAudioCallIntentResponse alloc] initWithCode:INStartAudioCallIntentResponseCodeReady userActivity:activity];
    completion(response);
}

- (void)handleStartAudioCall:(nonnull INStartAudioCallIntent *)intent completion:(nonnull void (^)(INStartAudioCallIntentResponse * _Nonnull))completion {
    NSUserActivity *activity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass(INStartAudioCallIntent.class)];
    INStartAudioCallIntentResponse *response = [[INStartAudioCallIntentResponse alloc] initWithCode:INStartAudioCallIntentResponseCodeContinueInApp userActivity:activity];
    completion(response);
}

@end
