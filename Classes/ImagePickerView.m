/* ImagePickerViewController.m
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

#import <MobileCoreServices/UTCoreTypes.h>
#import <AVFoundation/AVCaptureDevice.h>
#import <AVFoundation/AVFoundation.h>
#import "ImagePickerView.h"
#import "PhoneMainView.h"
#import "SVProgressHUD.h"
#import "ShareViewController.h"


@implementation ImagePickerView

@synthesize imagePickerDelegate;
@synthesize sourceType;
@synthesize mediaTypes;
@synthesize allowsEditing;
@synthesize popoverController;

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super init];
	if (self != nil) {
		pickerController = [[UIImagePickerController alloc] init];
		if (IPAD) {
			popoverController = [[UIPopoverController alloc] initWithContentViewController:pickerController];
		}
	}
	return self;
}

- (BOOL) shouldAutorotate{
	return NO;
}

#if __IPHONE_OS_VERSION_MAX_ALLOWED < 90000
- (NSUInteger)supportedInterfaceOrientations {
	return UIInterfaceOrientationMaskPortrait;
}
#else
- (UIInterfaceOrientationMask)supportedInterfaceOrientations{
	return UIInterfaceOrientationMaskPortrait;
}
#endif

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:nil];
		compositeDescription.darkBackground = false;
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];

	[self.view setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
	if (popoverController == nil) {
		[pickerController.view setFrame:[self.view bounds]];
		[self.view addSubview:[pickerController view]];
	} else {
		[popoverController setDelegate:self];
	}
	[pickerController setDelegate:self];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	if (popoverController != nil) {
		[popoverController presentPopoverFromRect:CGRectZero
										   inView:self.view
						 permittedArrowDirections:UIPopoverArrowDirectionAny
										 animated:FALSE];
	}
	[[UIApplication sharedApplication] setStatusBarHidden:NO]; // Fix UIImagePickerController status bar hide
	[[UIApplication sharedApplication]
		setStatusBarStyle:UIStatusBarStyleDefault]; // Fix UIImagePickerController status bar style change

	[PhoneMainView.instance hideStatusBar:YES];
	
	//Prevent rotation of camera
	NSNumber *value = [NSNumber numberWithInt:UIInterfaceOrientationPortrait];
	[[UIDevice currentDevice] setValue:value forKey:@"orientation"];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	if (popoverController != nil) {
		[popoverController dismissPopoverAnimated:NO];
	}

	[PhoneMainView.instance hideStatusBar:NO];
}

#pragma mark - Property Functions

- (BOOL)allowsEditing {
	return pickerController.allowsEditing;
}

- (void)setAllowsEditing:(BOOL)aallowsEditing {
	pickerController.allowsEditing = aallowsEditing;
}

- (UIImagePickerControllerSourceType)sourceType {
	return pickerController.sourceType;
}

- (void)setSourceType:(UIImagePickerControllerSourceType)asourceType {
	pickerController.sourceType = asourceType;
}

- (NSArray *)mediaTypes {
	return pickerController.mediaTypes;
}

- (void)setMediaTypes:(NSArray *)amediaTypes {
	pickerController.mediaTypes = amediaTypes;
}

#pragma mark -

- (void)dismiss {
	if ([[PhoneMainView.instance currentView] equal:ImagePickerView.compositeViewDescription]) {
		[PhoneMainView.instance popCurrentView];
	}
}

#pragma mark - UIImagePickerControllerDelegate Functions

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
	
	dispatch_async(dispatch_get_main_queue(), ^{
		
		[self dismiss];
		
		NSString *type = [info objectForKey:UIImagePickerControllerMediaType];
		if ([type isEqualToString:(NSString *)kUTTypeVideo] ||
			[type isEqualToString:(NSString *)kUTTypeMovie]) {
			NSURL *urlvideo = [info objectForKey:UIImagePickerControllerMediaURL];
			if(urlvideo != nil && self.imagePickerDelegate != nil) {
				[imagePickerDelegate imagePickerDelegateVideo:urlvideo info:info];
			}
		} else {
			NSURL *alassetURL = [info objectForKey:UIImagePickerControllerReferenceURL];
			PHAsset *phasset = nil;
			// when photo from camera, it hasn't be saved
			if (alassetURL) {
				PHFetchResult<PHAsset *> *phFetchResult = [PHAsset fetchAssetsWithALAssetURLs:@[alassetURL] options:nil];
				phasset = [phFetchResult firstObject];
			}
			
			UIImage *image = [info objectForKey:UIImagePickerControllerEditedImage] ? [info objectForKey:UIImagePickerControllerEditedImage] : [info objectForKey:UIImagePickerControllerOriginalImage];
			if (!phasset) {
				[self writeImageToGallery:image];
				return;
			}
			[self passImageToDelegate:image PHAssetId:[phasset localIdentifier]];
		}
	});
	
}


-(void) writeImageToGallery:(UIImage *)image {
	NSString *localIdentifier;
	[SVProgressHUD show];
	
	PHFetchResult<PHAssetCollection *> *assetCollections = [PHAssetCollection fetchAssetCollectionsWithType:PHAssetCollectionTypeAlbum subtype:PHAssetCollectionSubtypeAlbumRegular options:nil];
	__block PHObjectPlaceholder *placeHolder;

	for (PHAssetCollection *assetCollection in assetCollections) {
		if([[assetCollection localizedTitle] isEqualToString:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"]]  ){
			localIdentifier = assetCollection.localIdentifier;
			break;
		}
	}
	
	if(localIdentifier ){
		PHFetchResult *fetchResult = [PHAssetCollection fetchAssetCollectionsWithLocalIdentifiers:@[localIdentifier] options:nil];
		PHAssetCollection *assetCollection = fetchResult.firstObject;

		[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
			PHAssetChangeRequest *assetChangeRequest = [PHAssetChangeRequest creationRequestForAssetFromImage:image];
			PHAssetCollectionChangeRequest *assetCollectionChangeRequest = [PHAssetCollectionChangeRequest changeRequestForAssetCollection:assetCollection];
			[assetCollectionChangeRequest addAssets:@[[assetChangeRequest placeholderForCreatedAsset]]];
			placeHolder = [assetChangeRequest placeholderForCreatedAsset];
		} completionHandler:^(BOOL success, NSError *error) {
			dispatch_async(dispatch_get_main_queue(), ^{
			[SVProgressHUD dismiss];
			if (!success) {
				NSLog(@"Error creating asset: %@", error);
			} else {
				[self passImageToDelegate:image PHAssetId:[placeHolder localIdentifier]];
			}
			});
		}];
	}else{
		__block PHObjectPlaceholder *albumPlaceholder;
		[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
			PHAssetCollectionChangeRequest *changeRequest = [PHAssetCollectionChangeRequest creationRequestForAssetCollectionWithTitle:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"]];
			albumPlaceholder = changeRequest.placeholderForCreatedAssetCollection;
		} completionHandler:^(BOOL success, NSError *error) {
			if (success) {
				PHFetchResult *fetchResult = [PHAssetCollection fetchAssetCollectionsWithLocalIdentifiers:@[albumPlaceholder.localIdentifier] options:nil];
				PHAssetCollection *assetCollection = fetchResult.firstObject;
				
				[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
					PHAssetChangeRequest *assetChangeRequest = [PHAssetChangeRequest creationRequestForAssetFromImage:image];
					PHAssetCollectionChangeRequest *assetCollectionChangeRequest = [PHAssetCollectionChangeRequest changeRequestForAssetCollection:assetCollection];
					[assetCollectionChangeRequest addAssets:@[[assetChangeRequest placeholderForCreatedAsset]]];
					placeHolder = [assetChangeRequest placeholderForCreatedAsset];
				} completionHandler:^(BOOL success, NSError *error) {
					dispatch_async(dispatch_get_main_queue(), ^{

					[SVProgressHUD dismiss];
					if (!success) {
						NSLog(@"Error creating asset: %@", error);
					} else {
						[self passImageToDelegate:image PHAssetId:[placeHolder localIdentifier]];
					}
					});
				}];
			} else {
				[SVProgressHUD dismiss];
				NSLog(@"Error creating album: %@", error);
			}
		}];
		
	}
}


- (void) passImageToDelegate:(UIImage *)image PHAssetId:(NSString *)assetId {
	if (imagePickerDelegate != nil) {
		[imagePickerDelegate imagePickerDelegateImage:image info:(NSString *)assetId];
	}
}
/*
    if (imagePickerDelegate != nil) {
        [imagePickerDelegate imagePickerDelegateImage:image info:(__bridge NSDictionary *)contextInfo];
    }
}
*/
- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
	[self dismiss];
}

- (BOOL)popoverControllerShouldDismissPopover:(UIPopoverController *)apopoverController {
	[self dismiss];
	return TRUE;
}

- (void)navigationController:(UINavigationController *)navigationController
	  willShowViewController:(UIViewController *)viewController
					animated:(BOOL)animated {

	if ([navigationController isKindOfClass:[UIImagePickerController class]]) {
		[[UIApplication sharedApplication] setStatusBarHidden:NO]; // Fix UIImagePickerController status bar hide
		[[UIApplication sharedApplication]
			setStatusBarStyle:UIStatusBarStyleBlackOpaque]; // Fix UIImagePickerController status bar style change
	}
}

+ (void)SelectImageFromDevice:(id<ImagePickerDelegate>)delegate
				   atPosition:(UIView *)ipadPopoverView
					   inView:(UIView *)ipadView
	 withDocumentMenuDelegate:(id<UIDocumentMenuDelegate>)documentMenuDelegate {
	void (^block)(UIImagePickerControllerSourceType) = ^(UIImagePickerControllerSourceType type) {
	  ImagePickerView *view = VIEW(ImagePickerView);
	  view.sourceType = type;

	  // Displays a control that allows the user to choose picture or
	  // movie capture, if both are available:
	  view.mediaTypes = [NSArray arrayWithObjects:(NSString *)kUTTypeMovie,(NSString *)kUTTypeImage,nil];

	  // Hides the controls for moving & scaling pictures, or for
	  // trimming movies. To instead show the controls, use YES.
	  view.allowsEditing = NO;
	  view.imagePickerDelegate = delegate;

	  if (IPAD && ipadView && ipadPopoverView) {
		  UIView *iterview = ipadPopoverView;
		  CGRect ipadPopoverPosition = iterview.frame;
		  do {
			  ipadPopoverPosition =
				  [iterview.superview convertRect:ipadPopoverPosition toView:iterview.superview.superview];
			  iterview = iterview.superview;
		  } while (iterview && iterview.superview != ipadView);
		  [view.popoverController presentPopoverFromRect:ipadPopoverPosition
												  inView:ipadView
								permittedArrowDirections:UIPopoverArrowDirectionAny
												animated:FALSE];
	  } else {
		  [PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	  }
	};
    if ([PHPhotoLibrary authorizationStatus] == PHAuthorizationStatusAuthorized) {
        DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Select the source", nil)];
        if ([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera]) {
            [sheet addButtonWithTitle:NSLocalizedString(@"Camera", nil)
                                block:^() {
                                    if([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo] !=  AVAuthorizationStatusAuthorized ) {
                                        [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Camera's permission", nil) message:NSLocalizedString(@"Camera not authorized", nil) delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Continue", nil] show];
                                        return;
                                    }
                                    block(UIImagePickerControllerSourceTypeCamera);
                                }];
        }
        if ([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]) {
            [sheet addButtonWithTitle:NSLocalizedString(@"Photo library", nil)
                                block:^() {
                                    block(UIImagePickerControllerSourceTypePhotoLibrary);
                                }];
        }
		
		if (documentMenuDelegate) {
			[sheet addButtonWithTitle:NSLocalizedString(@"Document",nil) block:^(){
				[self pickDocumentForDelegate:documentMenuDelegate];
			}];
		}
        [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
        
        [sheet showInView:PhoneMainView.instance.view];
    } else {
        [PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if ([PHPhotoLibrary authorizationStatus] == PHAuthorizationStatusAuthorized) {
                    DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Select the source", nil)];
                    if ([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera]) {
                        [sheet addButtonWithTitle:NSLocalizedString(@"Camera", nil)
                                            block:^() {
                                                if([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo] !=  AVAuthorizationStatusAuthorized ) {
                                                    [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Camera's permission", nil) message:NSLocalizedString(@"Camera not authorized", nil) delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Continue", nil] show];
                                                    return;
                                                }
                                                block(UIImagePickerControllerSourceTypeCamera);
                                            }];
                    }
                    if ([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]) {
                        [sheet addButtonWithTitle:NSLocalizedString(@"Photo library", nil)
                                            block:^() {
                                                block(UIImagePickerControllerSourceTypePhotoLibrary);
                                            }];
                    }
					if (documentMenuDelegate) {
						[sheet addButtonWithTitle:NSLocalizedString(@"Document",nil) block:^(){
							[self pickDocumentForDelegate:documentMenuDelegate];
						}];
					}
                    [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
                    
                    [sheet showInView:PhoneMainView.instance.view];
                } else {
                    [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Photo's permission", nil) message:NSLocalizedString(@"Photo not authorized", nil) delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Continue", nil] show];
                }
            });
        }];
    }
}

+(void) pickDocumentForDelegate:(id<UIDocumentMenuDelegate>)documentMenuDelegate {
	UIDocumentMenuViewController *documentProviderMenu = [[UIDocumentMenuViewController alloc] initWithDocumentTypes:SUPPORTED_EXTENTIONS inMode:UIDocumentPickerModeImport];
	documentProviderMenu.delegate = documentMenuDelegate;
	[PhoneMainView.instance presentViewController:documentProviderMenu animated:YES completion:nil];
}

@end
