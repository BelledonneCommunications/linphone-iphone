/* ImageViewController.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "ImageViewController.h"
#import "PhoneMainView.h"

@interface ImageViewController ()

@end

@implementation ImageViewController

@synthesize scrollView;
@synthesize imageView;
@synthesize backButton;
@synthesize image;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ImageViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [scrollView release];
    [imageView release];
    [backButton release];
    [image release];
    
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"ImageView"
                                                                content:@"ImageViewController"
                                                               stateBar:nil
                                                        stateBarEnabled:false
                                                                 tabBar:nil
                                                          tabBarEnabled:false
                                                             fullscreen:false
                                                          landscapeMode:[LinphoneManager runningOnIpad]
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [scrollView setDelegate:self];
}


#pragma mark - Property Functions

- (void)setImage:(UIImage *)aimage {
    imageView.image = aimage;
    imageView.frame = CGRectMake(0, 0, aimage.size.width, aimage.size.height);
    scrollView.contentSize = aimage.size;
    [scrollView zoomToRect:imageView.bounds animated:FALSE];
    scrollView.minimumZoomScale = scrollView.zoomScale;
}

- (UIImage *)image {
    return imageView.image;
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)sender {
    if([[[PhoneMainView instance] currentView] equal:[ImageViewController compositeViewDescription]]) {
        [[PhoneMainView instance] popCurrentView];
    }
}


#pragma mark - UIScrollViewDelegate Functions

- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView {
    return self.imageView;
}

@end
