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
@synthesize linphoneIphoneVersionLabel;
@synthesize contentView;
@synthesize linkTapGestureRecognizer;
@synthesize linkLabel;


#pragma mark - Lifecycle Functions

- (id)init {
    self = [super initWithNibName:@"AboutViewController" bundle:[NSBundle mainBundle]];
    if (self != nil) {
        self->linkTapGestureRecognizer = [[UITapGestureRecognizer alloc] init];
    }
    return self;
}

- (void)dealloc {
    [linphoneCoreVersionLabel release];
    [linphoneIphoneVersionLabel release];
    [contentView release];
    [linkTapGestureRecognizer release];
    [linkLabel release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [linkTapGestureRecognizer addTarget:self action:@selector(onLinkTap:)];
    [linkLabel addGestureRecognizer:linkTapGestureRecognizer];
    
    UIScrollView *scrollView = (UIScrollView *)self.view;
    [scrollView addSubview:contentView];
    [scrollView setContentSize:[contentView bounds].size];
    
    [linphoneIphoneVersionLabel setText:[NSString stringWithFormat:@"Linphone iPhone %@", [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]]];

    [linphoneCoreVersionLabel setText:[NSString stringWithFormat:@"Linphone Core %s", linphone_core_get_version()]];
    
    [LinphoneUtils adjustFontSize:self.view mult:2.22f];
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


#pragma mark - Action Functions

- (IBAction)onLinkTap:(id)sender {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:linkLabel.text]];
}

@end
