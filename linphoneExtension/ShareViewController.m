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
    BOOL support = TRUE;
    // Inform the host that we're done, so it un-blocks its UI. Note: Alternatively you could call super's -didSelectPost, which will similarly complete the extension context.
    for (NSExtensionItem *item in self.extensionContext.inputItems) {
        for (NSItemProvider *provider in item.attachments) {
            NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:groupName];
            // TODO: Use [provider registeredTypeIdentifiersWithFileOptions:0]; to get all type identifiers of the provider instead of this if/else if structure
            support = TRUE;
            if ([provider hasItemConformingToTypeIdentifier:@"public.jpeg"]) {
                [self loadItem:provider typeIdentifier:@"public.jpeg" defaults:defaults];
            } else if ([provider hasItemConformingToTypeIdentifier:@"com.compuserve.gif"]) {
                [self loadItem:provider typeIdentifier:@"com.compuserve.gif" defaults:defaults];
            } else if ([provider hasItemConformingToTypeIdentifier:@"public.url"]) {
                [self loadItem:provider typeIdentifier:@"public.url" defaults:defaults];
            } else if ([provider hasItemConformingToTypeIdentifier:@"public.movie"]) {
                [self loadItem:provider typeIdentifier:@"public.movie" defaults:defaults];
            } else if ([provider hasItemConformingToTypeIdentifier:@"com.apple.mapkit.map-item"]) {
                [self loadItem:provider typeIdentifier:@"com.apple.mapkit.map-item" defaults:defaults];
            } else if ([provider hasItemConformingToTypeIdentifier:@"com.adobe.pdf"]) {
                [self loadItem:provider typeIdentifier:@"com.adobe.pdf" defaults:defaults];
            } else if ([provider hasItemConformingToTypeIdentifier:@"public.png"]) {
                [self loadItem:provider typeIdentifier:@"public.png" defaults:defaults];
            } else{
                NSLog(@"Unkown itemprovider = %@", provider);
                support = false;
            }
        }
    }
    if (!support)
         [self.extensionContext completeRequestReturningItems:@[] completionHandler:nil];
}

- (NSArray *)configurationItems {
    // To add configuration options via table cells at the bottom of the sheet, return an array of SLComposeSheetConfigurationItem here.
    return @[];
}

- (void)loadItem:(NSItemProvider *)provider typeIdentifier:(NSString *)typeIdentifier defaults:(NSUserDefaults *)defaults
{
    [provider loadItemForTypeIdentifier:typeIdentifier options:nil completionHandler:^(id<NSSecureCoding>  _Nullable item, NSError * _Null_unspecified error) {
        /*if([(NSObject*)item isKindOfClass:[NSDictionary class]]) {
            NSDictionary *dico = (NSDictionary *)item;
            if (dico) {
                return;
            }
        }*/
        if([(NSObject*)item isKindOfClass:[NSURL class]]) {
            NSURL *url = (NSURL *)item;
            NSData *nsData = [NSData dataWithContentsOfURL:url];
            
            if (nsData) {
                NSString *imgPath = url.path;
                NSString *filename = [imgPath lastPathComponent];
                if([imgPath containsString:@"var/mobile/Media/PhotoData"]) {
                    // We get the corresponding PHAsset identifier so we can display the image in the app without having to duplicate it.
                    NSDictionary *dict = @{@"nsData" : nsData,
                                           @"url" : filename};
                    [defaults setObject:dict forKey:@"photoData"];
                } else if ([imgPath containsString:@"var/mobile/Library/Mobile Documents/com~apple~CloudDocs"]) {
                    // shared files from icloud drive
                    NSDictionary *dict = @{@"nsData" : nsData,
                                           @"url" : filename};
                    [defaults setObject:dict forKey:@"icloudData"];
                }else  {
                    //Others
                    NSDictionary *dict = @{@"url" : [url absoluteString]};
                    [defaults setObject:dict forKey:@"url"];
                }
            } else {
                NSLog(@"NSExtensionItem Error, provider = %@", provider);
                [self.extensionContext completeRequestReturningItems:@[] completionHandler:nil];
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
        } else {
            //share text
            NSLog(@"Unsupported provider = %@", provider);
        }
    }];
}

@end
