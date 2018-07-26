//
//  IntentHandler.m
//  siriExtension
//
//  Created by David Idmansour on 18/07/2018.
//

#import "IntentHandler.h"

@interface IntentHandler () <INSendMessageIntentHandling, INStartAudioCallIntentHandling>

@end

@implementation IntentHandler

- (id)handlerForIntent:(INIntent *)intent {
    return self;
}

#pragma mark - INSendMessageIntentHandling

- (void)resolveRecipientsForSendMessage:(INSendMessageIntent *)intent withCompletion:(void (^)(NSArray<INPersonResolutionResult *> *resolutionResults))completion {
    NSArray<INPerson *> *contacts = intent.recipients;
    if (contacts.count == 0) {
        completion(@[[INPersonResolutionResult needsValue]]);
        return;
    }
    NSMutableArray<INPersonResolutionResult *> *responses = [NSMutableArray array];
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.belledonne-communications.linphone.siri"];
    NSDictionary<NSString *, NSArray *> *addresses = [defaults objectForKey:@"addresses"];
    for (id iterator in contacts)
        [responses addObject:[INPersonResolutionResult notRequired]];
    NSString *contactName = [contacts[0].displayName lowercaseString];
    NSMutableArray<INPerson *> *matchingContacts = [NSMutableArray array];
    for (NSString *name in addresses.allKeys) {
        if ([[name lowercaseString] containsString:[contactName lowercaseString]]) {
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
            response = [INPersonResolutionResult successWithResolvedPerson:matchingContacts.firstObject];
            break;
        default:{
            BOOL match = NO;
            for (INPerson *person in matchingContacts)
                if ([person.displayName isEqualToString:contactName]) {
                    response = [INPersonResolutionResult confirmationRequiredWithPersonToConfirm:person];
                    match = YES;
                }
            if (!match)
                response = [INPersonResolutionResult disambiguationWithPeopleToDisambiguate:matchingContacts];
        }
    }
    [responses setObject:response atIndexedSubscript:0];
    completion(responses);
}

- (void)resolveContentForSendMessage:(INSendMessageIntent *)intent withCompletion:(void (^)(INStringResolutionResult *resolutionResult))completion {
    NSString *text = intent.content;
    if (text && ![text isEqualToString:@""]) {
        completion([INStringResolutionResult successWithResolvedString:text]);
    } else {
        completion([INStringResolutionResult needsValue]);
    }
}

- (void)confirmSendMessage:(INSendMessageIntent *)intent completion:(void (^)(INSendMessageIntentResponse *response))completion {
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.belledonne-communications.linphone.siri"];
    NSNumber *loggedIn = [defaults objectForKey:@"loggedIn"];
    if (!loggedIn || [loggedIn isEqualToNumber:@NO]) {
        INSendMessageIntentResponse *response = [[INSendMessageIntentResponse alloc] initWithCode:INSendMessageIntentResponseCodeFailureRequiringAppLaunch userActivity:nil];
        completion(response);
        return;
    }
    
    NSUserActivity *userActivity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass([INSendMessageIntent class])];
    INSendMessageIntentResponse *response = [[INSendMessageIntentResponse alloc] initWithCode:INSendMessageIntentResponseCodeReady userActivity:userActivity];
    completion(response);
}

- (void)handleSendMessage:(INSendMessageIntent *)intent completion:(void (^)(INSendMessageIntentResponse *response))completion {
    NSUserActivity *userActivity = [[NSUserActivity alloc] initWithActivityType:NSStringFromClass([INSendMessageIntent class])];
    /*
     this solution requires to manually open the app in Siri since SiriKit
     doesn't provide a INSendMessageIntentResponseCodeContinueInApp code
    */
    INSendMessageIntentResponse *response = [[INSendMessageIntentResponse alloc] initWithCode:INSendMessageIntentResponseCodeInProgress userActivity:userActivity];
    completion(response);
}

#pragma mark - INStartAudioCallIntentHandling

- (void)resolveContactsForStartAudioCall:(INStartAudioCallIntent *)intent withCompletion:(void (^)(NSArray<INPersonResolutionResult *> * _Nonnull))completion {
    NSArray<INPerson *> *contacts = intent.contacts;
    if (contacts.count == 0) {
        completion(@[[INPersonResolutionResult needsValue]]);
        return;
    }
    NSMutableArray<INPersonResolutionResult *> *responses = [NSMutableArray array];
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.belledonne-communications.linphone.siri"];
    NSDictionary<NSString *, NSArray *> *addresses = [defaults objectForKey:@"addresses"];
    for (id iterator in contacts)
        [responses addObject:[INPersonResolutionResult notRequired]];
    NSString *contactName = [contacts[0].displayName lowercaseString];
    NSMutableArray<INPerson *> *matchingContacts = [NSMutableArray array];
    for (NSString *name in addresses.allKeys) {
        if ([[name lowercaseString] containsString:[contactName lowercaseString]]) {
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
            response = [INPersonResolutionResult successWithResolvedPerson:matchingContacts.firstObject];
            break;
        default:{
            BOOL match = NO;
            for (INPerson *person in matchingContacts)
                if ([person.displayName isEqualToString:contactName]) {
                    response = [INPersonResolutionResult confirmationRequiredWithPersonToConfirm:person];
                    match = YES;
                }
            if (!match)
            response = [INPersonResolutionResult disambiguationWithPeopleToDisambiguate:matchingContacts];
        }
    }
    [responses setObject:response atIndexedSubscript:0];
    completion(responses);
}

- (void)confirmStartAudioCall:(INStartAudioCallIntent *)intent completion:(void (^)(INStartAudioCallIntentResponse * _Nonnull))completion {
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:@"group.belledonne-communications.linphone.siri"];
    NSNumber *loggedIn = [defaults objectForKey:@"loggedIn"];
    if (!loggedIn || [loggedIn isEqualToNumber:@NO]) {
        INStartAudioCallIntentResponse *response = [[INStartAudioCallIntentResponse alloc] initWithCode:INStartAudioCallIntentResponseCodeFailureAppConfigurationRequired userActivity:nil];
        completion(response);
        return;
    }
    
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
