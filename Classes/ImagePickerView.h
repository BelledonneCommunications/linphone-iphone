/* ImagePickerViewController.h
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

#import "UICompositeView.h"

@protocol ImagePickerDelegate <NSObject>

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSDictionary *)info;

@end

@interface ImagePickerView : UIViewController <UICompositeViewDelegate, UINavigationControllerDelegate,
											   UIImagePickerControllerDelegate, UIPopoverControllerDelegate> {
  @private
	UIImagePickerController *pickerController;
}

@property(nonatomic, strong) id<ImagePickerDelegate> imagePickerDelegate;
@property(nonatomic) UIImagePickerControllerSourceType sourceType;
@property(nonatomic, copy) NSArray *mediaTypes;
@property(nonatomic) BOOL allowsEditing;
@property(nonatomic, readonly) UIPopoverController *popoverController;

+ (void)SelectImageFromDevice:(id<ImagePickerDelegate>)delegate
				   atPosition:(UIView *)ipadPopoverPosition
					   inView:(UIView *)view;

@end
