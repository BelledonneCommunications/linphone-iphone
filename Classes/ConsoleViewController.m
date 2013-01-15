/*ConsoleViewController.h
 *
 * Copyright (C) 2010  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */          


#import "ConsoleViewController.h"

@implementation ConsoleViewController

@synthesize logsView;



#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ConsoleViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [logsView release];
    
    [super dealloc];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"ConsoleView"
                                                                content:@"ConsoleViewController"
                                                               stateBar:@"UIStateBar"
                                                        stateBarEnabled:true
                                                                 tabBar:@"UIMainBar"
                                                          tabBarEnabled:true
                                                             fullscreen:false
                                                          landscapeMode:[LinphoneManager runningOnIpad]
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [logsView loadHTMLString:@"<html><body><pre id=\"content\"></pre></body><html>" baseURL:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneLogsUpdate
                                                  object:nil];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [logsView setDelegate:self];
    
    UIScrollView *scrollView = [ConsoleViewController defaultScrollView:logsView];
    UIEdgeInsets inset = {0, 0, 10, 0};
    [scrollView setContentInset:inset];
    [scrollView setScrollIndicatorInsets:inset];
    
    [scrollView setBounces:FALSE];
}


#pragma mark - UIWebViewDelegate Functions

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    NSString *logs = [[LinphoneManager instance].logs componentsJoinedByString:@"\n"];
    [self addLog:logs scroll:TRUE];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(logsUpdateEvent:)
                                                 name:kLinphoneLogsUpdate
                                               object:nil];
}


#pragma mark - Event Functions

- (void)logsUpdateEvent:(NSNotification*)notif {
    NSString *log = [notif.userInfo objectForKey: @"log"];
    [self addLog:log scroll:FALSE];
}


#pragma mark - 

+ (UIScrollView *)defaultScrollView:(UIWebView *)webView {
    UIScrollView *scrollView = nil;
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] >= 5.0) {
        return webView.scrollView;
    }  else {
        for (UIView *subview in [webView subviews]) {
            if ([subview isKindOfClass:[UIScrollView class]]) {
                scrollView = (UIScrollView *)subview;
            }
        }
    }
    return scrollView;
}

- (void)clear {
    NSString *js = @"document.getElementById('content').innerHTML += ''";
    [logsView stringByEvaluatingJavaScriptFromString:js];
}

- (void)addLog:(NSString*)log scroll:(BOOL)scroll {
    log = [log stringByReplacingOccurrencesOfString:@"\r" withString:@""];
    log = [log stringByReplacingOccurrencesOfString:@"\n" withString:@"\\n"];
    log = [log stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
    log = [log stringByReplacingOccurrencesOfString:@"&" withString:@"&amp;"];
    log = [log stringByReplacingOccurrencesOfString:@"<" withString:@"&lt;"];
    log = [log stringByReplacingOccurrencesOfString:@">" withString:@"&gt;"];
    NSMutableString *js = [NSMutableString stringWithFormat:@"document.getElementById('content').innerHTML += \"%@\\n\";", log];
    if(scroll) {
        [js appendString:@"window.scrollTo(0, document.body.scrollHeight);"];
    }
    [logsView stringByEvaluatingJavaScriptFromString:js];
}

@end
