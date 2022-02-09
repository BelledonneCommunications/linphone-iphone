/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
					// Send only one item
					return;
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

-(void) viewDidAppear:(BOOL)animated {
	[self didSelectPost];
}

- (NSArray *)configurationItems {
    // To add configuration options via table cells at the bottom of the sheet, return an array of SLComposeSheetConfigurationItem here.
    return @[];
}

- (NSString *)cacheDirectory {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString *cachePath = [paths objectAtIndex:0];
	BOOL isDir = NO;
	NSError *error;
	if (![[NSFileManager defaultManager] fileExistsAtPath:cachePath isDirectory:&isDir] && isDir == NO) {
		[[NSFileManager defaultManager] createDirectoryAtPath:cachePath
								  withIntermediateDirectories:NO
												   attributes:nil
														error:&error];
	}
	return cachePath;
}

-(void) nsDataWrite:(NSData *)data {
	NSString* groupName = [NSString stringWithFormat:@"group.%@",[[NSBundle mainBundle] bundleIdentifier]];
	NSError *error = nil;
	NSString *path  =[[[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:groupName] path];
	NSString *fullCacheFilePathPath = [NSString stringWithFormat:@"%@/%@",path,@"nsData"];
	[[NSFileManager defaultManager] removeItemAtURL:[NSURL fileURLWithPath:fullCacheFilePathPath] error:&error];
	if (![data writeToFile:fullCacheFilePathPath atomically:YES]) {
		NSLog(@"nsDataWrite error");
	}
}

- (void)loadItem:(NSItemProvider *)provider typeIdentifier:(NSString *)typeIdentifier defaults:(NSUserDefaults *)defaults  {
    [provider loadItemForTypeIdentifier:typeIdentifier options:nil completionHandler:^(id<NSSecureCoding>  _Nullable item, NSError * _Null_unspecified error) {
        if([(NSObject*)item isKindOfClass:[NSURL class]]) {
            NSURL *url = (NSURL *)item;
            NSData *nsData = [NSData dataWithContentsOfURL:url];
            
            if (nsData) {
                NSString *imgPath = url.path;
                NSString *filename = [imgPath lastPathComponent];
                if([imgPath containsString:@"var/mobile/Media/PhotoData"]) {
                    // We get the corresponding PHAsset identifier so we can display the image in the app without having to duplicate it.
                    NSDictionary *dict = @{@"url" : filename,
                                           @"message" : self.contentText};
					[self nsDataWrite:nsData];
                    [defaults setObject:dict forKey:@"photoData"];
                } else if ([imgPath containsString:@"var/mobile/Library/Mobile Documents/com~apple~CloudDocs"] || [[url scheme] isEqualToString:@"file"]) {
                    // shared files from icloud drive
                    NSDictionary *dict = @{@"url" : filename,
                                           @"message" : self.contentText};
					[self nsDataWrite:nsData];
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
            NSDictionary *dict = @{@"url" : [NSString stringWithFormat:@"IMAGE_%f.PNG", [[NSDate date] timeIntervalSince1970]],
                                   @"message" : self.contentText};
			[self nsDataWrite:UIImagePNGRepresentation(image)];
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
