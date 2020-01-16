//
//  IASKAppSettingsWebViewController.h
//  http://www.inappsettingskit.com
//
//  Copyright (c) 2010:
//  Luc Vandal, Edovia Inc., http://www.edovia.com
//  Ortwin Gentz, FutureTap GmbH, http://www.futuretap.com
//  All rights reserved.
// 
//  It is appreciated but not required that you give credit to Luc Vandal and Ortwin Gentz, 
//  as the original authors of this code. You can give credit in a blog post, a tweet or on 
//  a info page of your app. Also, the original authors appreciate letting them know if you use this code.
//
//  This code is licensed under the BSD license that is available at: http://www.opensource.org/licenses/bsd-license.php
//

#import "IASKAppSettingsWebViewController.h"
#pragma deploymate push "ignored-api-availability"
@implementation IASKAppSettingsWebViewController

@synthesize url;
@synthesize webView;

- (id)initWithFile:(NSString*)urlString key:(NSString*)key {
    self = [super init];
    if (self) {
        self.url = [NSURL URLWithString:urlString];
        if (!self.url || ![self.url scheme]) {
            NSString *path = [[NSBundle mainBundle] pathForResource:[urlString stringByDeletingPathExtension] ofType:[urlString pathExtension]];
            if(path)
                self.url = [NSURL fileURLWithPath:path];
            else
                self.url = nil;
        }
    }
    return self;
}

- (void)loadView
{
    webView = [[WKWebView alloc] init];
    webView.autoresizingMask = UIViewAutoresizingFlexibleWidth |
    UIViewAutoresizingFlexibleHeight;
    webView.navigationDelegate = self;
    
    self.view = webView;
}

- (void)dealloc {
	webView = nil;
	url = nil;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[webView loadRequest:[NSURLRequest requestWithURL:self.url]];
}

- (void)viewDidUnload {
	[super viewDidUnload];
	self.webView = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return YES;
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    [self.webView evaluateJavaScript:@"document.title" completionHandler:^(id result, NSError * error) {
        if (error == nil) {
            if (result != nil) {
                self.navigationItem.title = [NSString stringWithFormat:@"%@", result];
            }
        } else {
            NSLog(@"evaluateJavaScript error : %@", error.localizedDescription);
        }
    }];
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    NSURL *newURL = [navigationAction.request URL];
    
    // intercept mailto URL and send it to an in-app Mail compose view instead
    if ([[newURL scheme] isEqualToString:@"mailto"]) {

        NSArray *rawURLparts = [[newURL resourceSpecifier] componentsSeparatedByString:@"?"];
        if (rawURLparts.count > 2) {
            decisionHandler(WKNavigationActionPolicyCancel); // invalid URL
        }
        
        MFMailComposeViewController *mailViewController = [[MFMailComposeViewController alloc] init];
        mailViewController.mailComposeDelegate = self;

        NSMutableArray *toRecipients = [NSMutableArray array];
        NSString *defaultRecipient = [rawURLparts objectAtIndex:0];
        if (defaultRecipient.length) {
            [toRecipients addObject:defaultRecipient];
        }
        
        if (rawURLparts.count == 2) {
            NSString *queryString = [rawURLparts objectAtIndex:1];
            
            NSArray *params = [queryString componentsSeparatedByString:@"&"];
            for (NSString *param in params) {
                NSArray *keyValue = [param componentsSeparatedByString:@"="];
                if (keyValue.count != 2) {
                    continue;
                }
                NSString *key = [[keyValue objectAtIndex:0] lowercaseString];
                NSString *value = [keyValue objectAtIndex:1];

                value = (NSString *)CFBridgingRelease(CFURLCreateStringByReplacingPercentEscapesUsingEncoding(
                    kCFAllocatorDefault, (CFStringRef)value, CFSTR(""), kCFStringEncodingUTF8));

                if ([key isEqualToString:@"subject"]) {
                    [mailViewController setSubject:value];
                }
                
                if ([key isEqualToString:@"body"]) {
                    [mailViewController setMessageBody:value isHTML:NO];
                }
                
                if ([key isEqualToString:@"to"]) {
                    [toRecipients addObjectsFromArray:[value componentsSeparatedByString:@","]];
                }
                
                if ([key isEqualToString:@"cc"]) {
                    NSArray *recipients = [value componentsSeparatedByString:@","];
                    [mailViewController setCcRecipients:recipients];
                }
                
                if ([key isEqualToString:@"bcc"]) {
                    NSArray *recipients = [value componentsSeparatedByString:@","];
                    [mailViewController setBccRecipients:recipients];
                }
            }
        }
        
        [mailViewController setToRecipients:toRecipients];

        [self presentModalViewController:mailViewController animated:YES];
        decisionHandler(WKNavigationActionPolicyCancel);
    }
    
    // open inline if host is the same, otherwise, pass to the system
    if (![newURL host] || [[newURL host] isEqualToString:[self.url host]]) {
        decisionHandler(WKNavigationActionPolicyAllow);
    }
    [[UIApplication sharedApplication] openURL:newURL];
    decisionHandler(WKNavigationActionPolicyCancel);
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error {
	[self dismissModalViewControllerAnimated:YES];
}
#pragma deploymate pop

@end
