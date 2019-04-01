//
//  ShareViewController.m
//  linphoneExtension
//
//  Created by Danmei Chen on 31/05/2018.
//

#import "ShareViewController.h"

@interface ShareViewController ()

@end

@implementation ShareViewController

- (BOOL)isContentValid {
    // Do validation of contentText and/or NSExtensionContext attachments here
    return YES;
}


- (void)didSelectPost {
    NSString* groupName = [NSString stringWithFormat:@"group.%@",[[NSBundle mainBundle] bundleIdentifier]];
	NSLog(@"[SHARE EXTENSTION] using group name inside EXTENSION %@",groupName);
    // This is called after the user selects Post. Do the upload of contentText and/or NSExtensionContext attachments.
    BOOL support = TRUE;
    // Inform the host that we're done, so it un-blocks its UI. Note: Alternatively you could call super's -didSelectPost, which will similarly complete the extension context.
    for (NSExtensionItem *item in self.extensionContext.inputItems) {
        for (NSItemProvider *provider in item.attachments) {
            NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:groupName];
            // TODO: Use [provider registeredTypeIdentifiersWithFileOptions:0]; to get all type identifiers of the provider instead of this if/else if structure
            support = TRUE;
			bool found = false;
			for (NSString *ti in SUPPORTED_EXTENTIONS) {
				if ([provider hasItemConformingToTypeIdentifier:ti]) {
					found=true;
					[self loadItem:provider typeIdentifier:ti defaults:defaults];
				}
			}
			if (!found){
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
        if([(NSObject*)item isKindOfClass:[NSURL class]]) {
            NSURL *url = (NSURL *)item;
            NSData *nsData = [NSData dataWithContentsOfURL:url];
            
            if (nsData) {
                NSString *imgPath = url.path;
                NSString *filename = [imgPath lastPathComponent];
                if([imgPath containsString:@"var/mobile/Media/PhotoData"]) {
                    // We get the corresponding PHAsset identifier so we can display the image in the app without having to duplicate it.
                    NSDictionary *dict = @{@"nsData" : nsData,
                                           @"url" : filename,
                                           @"message" : self.contentText};
                    [defaults setObject:dict forKey:@"photoData"];
                } else if ([imgPath containsString:@"var/mobile/Library/Mobile Documents/com~apple~CloudDocs"] || [[url scheme] isEqualToString:@"file"]) {
                    // shared files from icloud drive
                    NSDictionary *dict = @{@"nsData" : nsData,
                                           @"url" : filename,
                                           @"message" : self.contentText};
                    [defaults setObject:dict forKey:@"icloudData"];
                } else {
                    NSDictionary *dict = @{@"url" : [url absoluteString],
                                           @"message" : self.contentText};
                    [defaults setObject:dict forKey:@"url"];
                }
            } else {
                //Others
                NSDictionary *dict = @{@"url" : [url absoluteString],
                                       @"message" : self.contentText};
                [defaults setObject:dict forKey:@"url"];
            }

            [self respondUrl:defaults];
        } else if ([(NSObject*)item isKindOfClass:[UIImage class]]) {
            UIImage *image = (UIImage*)item;
            NSDictionary *dict = @{@"nsData" : UIImagePNGRepresentation(image),
                                   @"url" : [NSString stringWithFormat:@"IMAGE_%f.PNG", [[NSDate date] timeIntervalSince1970]],
                                   @"message" : self.contentText};
            [defaults setObject:dict forKey:@"photoData"];
            
            [self respondUrl:defaults];
        } else {
            //share text
            NSLog(@"Unsupported provider = %@", provider);
            [self.extensionContext completeRequestReturningItems:@[] completionHandler:nil];
        }
    }];
}

- (void)respondUrl:(NSUserDefaults *)defaults {
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
}

@end
