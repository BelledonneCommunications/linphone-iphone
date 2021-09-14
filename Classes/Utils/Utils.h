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

#import "LinphoneManager.h"

#define IPAD (LinphoneManager.runningOnIpad)
#define ANIMATED ([LinphoneManager.instance lpConfigBoolForKey:@"animations_preference"])
#define LC ([LinphoneManager getLc])
#define UIColorFromRGB(rgbValue) \
[UIColor colorWithRed:((float)((rgbValue & 0xFF0000) >> 16))/255.0 \
				green:((float)((rgbValue & 0x00FF00) >>  8))/255.0 \
				 blue:((float)((rgbValue & 0x0000FF) >>  0))/255.0 \
				alpha:1.0]

@interface LinphoneUtils : NSObject

+ (BOOL)findAndResignFirstResponder:(UIView*)view;
+ (void)adjustFontSize:(UIView*)view mult:(float)mult;
+ (void)buttonFixStates:(UIButton*)button;
+ (void)buttonMultiViewAddAttributes:(NSMutableDictionary*)attributes button:(UIButton*)button;
+ (void)buttonMultiViewApplyAttributes:(NSDictionary*)attributes button:(UIButton*)button;
+ (NSString *)deviceModelIdentifier;
+ (UIImage *)resizeImage:(UIImage *)imageToResize newSize:(CGSize)newSize;

+ (LinphoneAddress *)normalizeSipOrPhoneAddress:(NSString *)addr;
+ (UIAlertController *)networkErrorView:(NSString *)action;

typedef enum {
	LinphoneDateHistoryList,
	LinphoneDateHistoryDetails,
	LinphoneDateChatList,
	LinphoneDateChatBubble,
} LinphoneDateFormat;

+ (NSString *)timeToString:(time_t)time withFormat:(LinphoneDateFormat)format;

+ (BOOL)hasSelfAvatar;
+ (UIImage *)selfAvatar;

+ (NSString *)durationToString:(int)duration;
+ (NSString *)intervalToString:(NSTimeInterval)interval ;

+ (NSMutableDictionary <NSString *, PHAsset *> *)photoAssetsDictionary;

+ (NSArray *)parseRecordingName:(NSString *)filename;

@end

@interface NSNumber (HumanReadableSize)

- (NSString*)toHumanReadableSize;

@end

@interface UIImage (systemIcons)

+ (UIImage *)imageFromSystemBarButton:(UIBarButtonSystemItem)systemItem :(UIColor *) color;

@end

@interface UIImageView (ImageWithTint)

- (void)setImageNamed:(NSString *)name tintColor:(UIColor *)color;
- (void)setImageNamed:(NSString *)name tintColorLetter:(NSString *)letter;

@end

@interface NSString (linphoneExt)

- (NSString *)md5;
- (BOOL)containsSubstring:(NSString *)str;

@end

@interface UIImage (squareCrop)

- (UIImage *)squareCrop;
- (UIImage *)scaleToSize:(CGSize)size squared:(BOOL)squared;
+ (UIImage *)resizeImage:(UIImage *)image withMaxWidth:(float)maxWidth andMaxHeight:(float)maxHeight;

@end

@interface ContactDisplay : NSObject
+ (void)setDisplayNameLabel:(UILabel *)label forContact:(Contact *)contact;
+ (void)setDisplayNameLabel:(UILabel *)label forAddress:(const LinphoneAddress *)addr;
+ (void)setDisplayNameLabel:(UILabel *)label forAddress:(const LinphoneAddress *)addr withAddressLabel:(UILabel*)addressLabel;
@end

#import <UIKit/UIColor.h>
#import <UIKit/UIKit.h>

#define LINPHONE_MAIN_COLOR [UIColor colorWithRed:207.0f / 255.0f green:76.0f / 255.0f blue:41.0f / 255.0f alpha:1.0f]
#define LINPHONE_SETTINGS_BG_IOS7 [UIColor colorWithRed:164 / 255. green:175 / 255. blue:183 / 255. alpha:1.0]

@interface UIColor (LightAndDark)

- (UIColor *)adjustHue:(float)hm saturation:(float)sm brightness:(float)bm alpha:(float)am;

- (UIColor *)lumColor:(float)mult;

- (UIColor *)lighterColor;

- (UIColor *)darkerColor;

+(UIColor *)color:(NSString *)letter;

@end

@interface UIImage (ForceDecode)

+ (UIImage *)decodedImageWithImage:(UIImage *)image;

@end

@interface UIImage (ResizeAndThumbnail)

+ (UIImage *)UIImageThumbnail:(UIImage *)image thumbSize:(CGFloat) tbSize;

+ (UIImage *)UIImageResize:(UIImage *)image toSize:(CGSize) newSize;

+ (CGImageRef)resizeCGImage:(CGImageRef)image toWidth:(int)width andHeight:(int)height;

@end

/* Use that macro when you want to invoke a custom initialisation method on your class,
 whatever is using it (xib, source code, etc., tableview cell) */
#define INIT_WITH_COMMON_C                                                                                             \
	-(instancetype)init {                                                                                              \
		return [[super init] commonInit];                                                                              \
	}                                                                                                                  \
	-(instancetype)initWithCoder : (NSCoder *)aDecoder {                                                               \
		return [[super initWithCoder:aDecoder] commonInit];                                                            \
	}                                                                                                                  \
	-(instancetype)commonInit

#define INIT_WITH_COMMON_CF                                                                                            \
	-(instancetype)initWithFrame : (CGRect)frame {                                                                     \
		return [[super initWithFrame:frame] commonInit];                                                               \
	}                                                                                                                  \
	INIT_WITH_COMMON_C
