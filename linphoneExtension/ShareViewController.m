//
//  ShareViewController.m
//  linphoneExtension
//
//  Created by Danmei Chen on 31/05/2018.
//

#import "ShareViewController.h"

@interface ShareViewController ()

@end
static NSString* groupName = @"group.belledonne-communications.linphone";
@implementation ShareViewController

- (BOOL)isContentValid {
    // Do validation of contentText and/or NSExtensionContext attachments here
    return YES;
}

- (void)didSelectPost {
    // This is called after the user selects Post. Do the upload of contentText and/or NSExtensionContext attachments.
    
    // Inform the host that we're done, so it un-blocks its UI. Note: Alternatively you could call super's -didSelectPost, which will similarly complete the extension context.
    for (NSExtensionItem *item in self.extensionContext.inputItems) {
        for (NSItemProvider *provider in item.attachments) {
            NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:groupName];
        
            NSString *typeIdentifier;
            if ([provider hasItemConformingToTypeIdentifier:@"public.jpeg"]) {
                [self loadItem:provider typeIdentifier:@"public.jpeg" defaults:defaults key:@"img"];
            } else if ([provider hasItemConformingToTypeIdentifier:@"public.url"]) {
                [self loadItem:provider typeIdentifier:@"public.url" defaults:defaults key:@"web"];
            } else if ([provider hasItemConformingToTypeIdentifier:@"public.movie"]) {
                [self loadItem:provider typeIdentifier:@"public.movie" defaults:defaults key:@"file"];
            } else if ([provider hasItemConformingToTypeIdentifier:@"public.plain-text"]) {
                [self loadItem:provider typeIdentifier:@"public.plain-text" defaults:defaults key:@"text"];
            } else  if ([provider hasItemConformingToTypeIdentifier:@"com.adobe.pdf"]) {
                [self loadItem:provider typeIdentifier:@"com.adobe.pdf" defaults:defaults key:@"file"];
            } else{
                NSLog(@"Unkown itemprovider = %@", provider);
                typeIdentifier = nil;
            }
        }
    }
}

- (NSArray *)configurationItems {
    // To add configuration options via table cells at the bottom of the sheet, return an array of SLComposeSheetConfigurationItem here.
    return @[];
}

- (void)loadItem:(NSItemProvider *)provider typeIdentifier:(NSString *)typeIdentifier defaults:(NSUserDefaults *)defaults key:(NSString *)key {
    [provider loadItemForTypeIdentifier:typeIdentifier options:nil completionHandler:^(id<NSSecureCoding>  _Nullable item, NSError * _Null_unspecified error) {
        if([(NSObject*)item isKindOfClass:[NSURL class]]) {
            NSData *nsData = [NSData dataWithContentsOfURL:(NSURL*)item];
            if (nsData) {
                NSDictionary *dict = @{@"nsData" : nsData,
                                       @"url" : [(NSURL*)item absoluteString],
                                       @"name" : self.contentText};
                [defaults setObject:dict forKey:key];
            } else {
                NSLog(@"NSExtensionItem Error, provider = %@", provider);
                [self.extensionContext completeRequestReturningItems:@[] completionHandler:nil];
                return;
            }
        } else {
            NSDictionary *dict = @{@"name" : self.contentText};
            [defaults setObject:dict forKey:key];
        }
        
        UIResponder *responder = self;
        while (responder != nil) {
            if ([responder respondsToSelector:@selector(openURL:)]) {
                [responder performSelector:@selector(openURL:)
                                withObject:[NSURL URLWithString:@"message-linphone://" ]];
                [self.extensionContext completeRequestReturningItems:@[] completionHandler:nil];
                break;
            }
            responder = [responder nextResponder];
        }
        [defaults synchronize];
    }];
}

@end
