/* ContactDetailsImagePickerController.m
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "ImagePickerViewController.h"
#import "PhoneMainView.h"
#import "DTActionSheet.h"

@implementation ImagePickerViewController

@synthesize imagePickerDelegate;


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"ImagePicker"
                                                                content:@"ImagePickerViewController"
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


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setDelegate:self];
}


#pragma mark -

- (void)dismiss {
    if([[[PhoneMainView instance] currentView] equal:[ImagePickerViewController compositeViewDescription]]) {
        [[PhoneMainView instance] popCurrentView];
    }
}

+ (void)promptSelectSource:(void (^)(UIImagePickerControllerSourceType))block {
    DTActionSheet *sheet = [[[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Select picture source",nil)] autorelease];
    if([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera]) {
	    [sheet addButtonWithTitle:NSLocalizedString(@"Camera",nil) block:^(){
            block(UIImagePickerControllerSourceTypeCamera);
        }];
	}
	if([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]) {
	    [sheet addButtonWithTitle:NSLocalizedString(@"Photo library",nil) block:^(){
            block(UIImagePickerControllerSourceTypePhotoLibrary);
        }];
	}
    [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel",nil)];
    
    [sheet showInView:[PhoneMainView instance].view];
}


#pragma mark - UIImagePickerControllerDelegate Functions

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
    UIImage *image = [info objectForKey:UIImagePickerControllerEditedImage];
    if(image == nil) {
        image = [info objectForKey:UIImagePickerControllerOriginalImage];
    }
    if(image != nil && imagePickerDelegate != nil) {
        [imagePickerDelegate imagePickerDelegateImage:image];
    }
    [self dismiss];
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
    [self dismiss];
}

@end
