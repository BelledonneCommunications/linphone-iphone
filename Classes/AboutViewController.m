/* AboutViewController.m
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */          

#import "AboutViewController.h"
#include "ConsoleViewController.h"
#import "LinphoneManager.h"
#include "lpconfig.h"

@implementation AboutViewController

@synthesize linphoneCoreVersionLabel;
@synthesize linphoneLabel;
@synthesize linphoneIphoneVersionLabel;
@synthesize contentView;
@synthesize linkTapGestureRecognizer;
@synthesize linkLabel;
@synthesize licensesView;
@synthesize licenseLabel;
@synthesize copyrightLabel;


#pragma mark - Lifecycle Functions

- (id)init {
    self = [super initWithNibName:@"AboutViewController" bundle:[NSBundle mainBundle]];
    if (self != nil) {
        self->linkTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onLinkTap:)];
    }
    return self;
}

- (void)dealloc {
    [linphoneCoreVersionLabel release];
    [linphoneIphoneVersionLabel release];
    [contentView release];
    [linkTapGestureRecognizer release];
    [linkLabel release];
    [licensesView release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
	[linkLabel setText:NSLocalizedString(@"http://www.linphone.org", nil)];
	[licenseLabel setText:NSLocalizedString(@"GNU General Public License V2 ", nil)];
	[copyrightLabel setText:NSLocalizedString(@"© 2010-2012 Belledonne Communications ", nil)];
	
    [linkLabel addGestureRecognizer:linkTapGestureRecognizer];
    
    UIScrollView *scrollView = (UIScrollView *)self.view;
    [scrollView addSubview:contentView];
    [scrollView setContentSize:[contentView bounds].size];
    
	[linphoneLabel setText:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"]];
	
    [linphoneIphoneVersionLabel setText:[NSString stringWithFormat:@"%@ iPhone %@", [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"]
	 ,[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]]];

    [linphoneCoreVersionLabel setText:[NSString stringWithFormat:@"%@ Core %s", [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"], linphone_core_get_version()]];
    
    if([LinphoneManager runningOnIpad]) {
        [LinphoneUtils adjustFontSize:self.view mult:2.22f];
    }
    
    [AboutViewController removeBackground:licensesView];
    
    // Create a request to the resource
    NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL fileURLWithPath:[LinphoneManager bundleFile:@"licenses.html"]]] ;
    // Load the resource using the request
    [licensesView setDelegate:self];
    [licensesView loadRequest:request];
    [[AboutViewController defaultScrollView:licensesView] setScrollEnabled:FALSE];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"About"
                                                                content:@"AboutViewController"
                                                               stateBar:nil
                                                        stateBarEnabled:false
                                                                 tabBar:@"UIMainBar"
                                                          tabBarEnabled:true
                                                             fullscreen:false
                                                          landscapeMode:[LinphoneManager runningOnIpad]
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark -

+ (void)removeBackground:(UIView *)view {
    for (UIView *subview in [view subviews]) {
        [subview setOpaque:NO];
        [subview setBackgroundColor:[UIColor clearColor]];
    }
    [view setOpaque:NO];
    [view setBackgroundColor:[UIColor clearColor]];
}

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


#pragma mark - Action Functions

- (IBAction)onLinkTap:(id)sender {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:linkLabel.text]];
}


#pragma mark - UIWebViewDelegate Functions

- (void)webViewDidFinishLoad:(UIWebView *)webView {
    CGSize size = [webView sizeThatFits:CGSizeMake(self.view.bounds.size.width, 10000.0f)];
    float diff = size.height - webView.bounds.size.height;
    
    UIScrollView *scrollView = (UIScrollView *)self.view;
    CGRect contentFrame = [contentView bounds];
    contentFrame.size.height += diff;
    [contentView setAutoresizesSubviews:FALSE];
    [contentView setFrame:contentFrame];
    [contentView setAutoresizesSubviews:TRUE];
    [scrollView setContentSize:contentFrame.size];
    CGRect licensesViewFrame = [licensesView frame];
    licensesViewFrame.size.height += diff;
    [licensesView setFrame:licensesViewFrame];
}

- (BOOL)webView:(UIWebView *)inWeb shouldStartLoadWithRequest:(NSURLRequest *)inRequest navigationType:(UIWebViewNavigationType)inType {
    if (inType == UIWebViewNavigationTypeLinkClicked) {
        [[UIApplication sharedApplication] openURL:[inRequest URL]];
        return NO;
    }
    
    return YES;
}


@end
