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

#import <UIKit/UIView.h>
#import <CommonCrypto/CommonDigest.h>
#import <sys/utsname.h>
#import <AssetsLibrary/ALAsset.h>

#import "Utils.h"
#import "linphone/linphonecore.h"
#import "UILabel+Boldify.h"
#import "FastAddressBook.h"
#import "ColorSpaceUtilities.h"

@implementation LinphoneUtils


+ (UIImage *)resizeImage:(UIImage *)imageToResize newSize:(CGSize)newSize {
	UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:newSize];
	UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext*_Nonnull myContext) {
		[imageToResize drawInRect:(CGRect) {.origin = CGPointZero, .size = newSize}];
	}];
	return [image imageWithRenderingMode:imageToResize.renderingMode];
}

+ (BOOL)hasSelfAvatar {
	return [LinphoneManager.instance lpConfigStringForKey:@"avatar"] != nil;
}
+ (UIImage *)selfAvatar {
    return [LinphoneManager.instance avatar];
}

+ (NSString *)durationToString:(int)duration {
	NSMutableString *result = [[NSMutableString alloc] init];
	if (duration / 3600 > 0) {
		[result appendString:[NSString stringWithFormat:@"%02i:", duration / 3600]];
		duration = duration % 3600;
	}
	return [result stringByAppendingString:[NSString stringWithFormat:@"%02i:%02i", (duration / 60), (duration % 60)]];
}

+ (NSString *) intervalToString:(NSTimeInterval)interval {
	NSDateComponentsFormatter *formatter = [[NSDateComponentsFormatter alloc] init];
	formatter.allowedUnits = NSCalendarUnitSecond;
	formatter.unitsStyle = NSDateComponentsFormatterUnitsStyleAbbreviated;
	formatter.zeroFormattingBehavior = NSDateComponentsFormatterZeroFormattingBehaviorDropAll;
	return [formatter stringFromTimeInterval:interval];
}


+ (NSMutableDictionary <NSString *, PHAsset *> *)photoAssetsDictionary {
    NSMutableDictionary <NSString *, PHAsset *> *assetDict = [NSMutableDictionary dictionary];
    
    PHFetchOptions *options = [[PHFetchOptions alloc] init];
    [options setIncludeHiddenAssets:YES];
    [options setIncludeAllBurstAssets:YES];
    
    PHFetchResult *fetchRes = [PHAsset fetchAssetsWithOptions:options];
    
    for (PHAsset *asset in fetchRes) {
        NSString *key = [asset valueForKey:@"filename"];
        [assetDict setObject:asset forKey:[[key componentsSeparatedByString:@"."] firstObject]];
    }
    
    return assetDict;
}

+ (NSString *)timeToString:(time_t)time withFormat:(LinphoneDateFormat)format {
	NSString *formatstr;
	NSDate *todayDate = [[NSDate alloc] init];
	NSDate *messageDate = (time == 0) ? todayDate : [NSDate dateWithTimeIntervalSince1970:time];
	NSDateComponents *todayComponents =
		[[NSCalendar currentCalendar] components:NSCalendarUnitDay | NSCalendarUnitMonth | NSCalendarUnitYear
										fromDate:todayDate];
	NSDateComponents *dateComponents =
		[[NSCalendar currentCalendar] components:NSCalendarUnitDay | NSCalendarUnitMonth | NSCalendarUnitYear
										fromDate:messageDate];
	BOOL sameYear = (todayComponents.year == dateComponents.year);
	BOOL sameMonth = (sameYear && (todayComponents.month == dateComponents.month));
	BOOL sameDay = (sameMonth && (todayComponents.day == dateComponents.day));

	switch (format) {
		case LinphoneDateHistoryList:
			if (sameYear) {
				formatstr = NSLocalizedString(@"EEE dd MMMM",
											  @"Date formatting in History List, for current year (also see "
											  @"http://cybersam.com/ios-dev/quick-guide-to-ios-dateformatting)");
			} else {
				formatstr = NSLocalizedString(@"EEE dd MMMM yyyy",
											  @"Date formatting in History List, for previous years (also see "
											  @"http://cybersam.com/ios-dev/quick-guide-to-ios-dateformatting)");
			}
			break;
		case LinphoneDateHistoryDetails:
			formatstr = NSLocalizedString(@"EEE dd MMM 'at' HH'h'mm", @"Date formatting in History Details (also see "
																	  @"http://cybersam.com/ios-dev/"
																	  @"quick-guide-to-ios-dateformatting)");
			break;
		case LinphoneDateChatList:
			if (sameDay) {
				formatstr = NSLocalizedString(
					@"HH:mm", @"Date formatting in Chat List and Conversation bubbles, for current day (also see "
							  @"http://cybersam.com/ios-dev/quick-guide-to-ios-dateformatting)");
			} else {
				formatstr =
					NSLocalizedString(@"MM/dd", @"Date formatting in Chat List, for all but current day (also see "
												@"http://cybersam.com/ios-dev/quick-guide-to-ios-dateformatting)");
			}
			break;
		case LinphoneDateChatBubble:
			if (sameDay) {
				formatstr = NSLocalizedString(
					@"HH:mm", @"Date formatting in Chat List and Conversation bubbles, for current day (also see "
							  @"http://cybersam.com/ios-dev/quick-guide-to-ios-dateformatting)");
			} else {
				formatstr = NSLocalizedString(@"MM/dd - HH:mm",
											  @"Date formatting in Conversation bubbles, for all but current day (also "
											  @"see http://cybersam.com/ios-dev/quick-guide-to-ios-dateformatting)");
			}
			break;
	}
	NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
	[dateFormatter setDateFormat:formatstr];
	return [dateFormatter stringFromDate:messageDate];
}

+ (BOOL)findAndResignFirstResponder:(UIView *)view {
	if (view.isFirstResponder) {
		[view resignFirstResponder];
		return YES;
	}
	for (UIView *subView in view.subviews) {
		if ([LinphoneUtils findAndResignFirstResponder:subView])
			return YES;
	}
	return NO;
}

+ (void)adjustFontSize:(UIView *)view mult:(float)mult {
	if ([view isKindOfClass:[UILabel class]]) {
		UILabel *label = (UILabel *)view;
		UIFont *font = [label font];
		[label setFont:[UIFont fontWithName:font.fontName size:font.pointSize * mult]];
	} else if ([view isKindOfClass:[UITextField class]]) {
		UITextField *label = (UITextField *)view;
		UIFont *font = [label font];
		[label setFont:[UIFont fontWithName:font.fontName size:font.pointSize * mult]];
	} else if ([view isKindOfClass:[UIButton class]]) {
		UIButton *button = (UIButton *)view;
		UIFont *font = button.titleLabel.font;
		[button.titleLabel setFont:[UIFont fontWithName:font.fontName size:font.pointSize * mult]];
	} else {
		for (UIView *subView in [view subviews]) {
			[LinphoneUtils adjustFontSize:subView mult:mult];
		}
	}
}

+ (void)buttonFixStates:(UIButton *)button {
	// Interface builder lack fixes
	[button setTitle:[button titleForState:UIControlStateSelected]
			forState:(UIControlStateHighlighted | UIControlStateSelected)];
	[button setTitleColor:[button titleColorForState:UIControlStateHighlighted]
				 forState:(UIControlStateHighlighted | UIControlStateSelected)];
	[button setTitle:[button titleForState:UIControlStateSelected]
			forState:(UIControlStateDisabled | UIControlStateSelected)];
	[button setTitleColor:[button titleColorForState:UIControlStateDisabled]
				 forState:(UIControlStateDisabled | UIControlStateSelected)];
}

+ (void)buttonMultiViewAddAttributes:(NSMutableDictionary *)attributes button:(UIButton *)button {
	[LinphoneUtils addDictEntry:attributes item:[button titleForState:UIControlStateNormal] key:@"title-normal"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleForState:UIControlStateHighlighted]
							key:@"title-highlighted"];
	[LinphoneUtils addDictEntry:attributes item:[button titleForState:UIControlStateDisabled] key:@"title-disabled"];
	[LinphoneUtils addDictEntry:attributes item:[button titleForState:UIControlStateSelected] key:@"title-selected"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleForState:UIControlStateDisabled | UIControlStateHighlighted]
							key:@"title-disabled-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleForState:UIControlStateSelected | UIControlStateHighlighted]
							key:@"title-selected-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleForState:UIControlStateSelected | UIControlStateDisabled]
							key:@"title-selected-disabled"];

	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateNormal]
							key:@"title-color-normal"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateHighlighted]
							key:@"title-color-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateDisabled]
							key:@"title-color-disabled"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateSelected]
							key:@"title-color-selected"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateDisabled | UIControlStateHighlighted]
							key:@"title-color-disabled-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateSelected | UIControlStateHighlighted]
							key:@"title-color-selected-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button titleColorForState:UIControlStateSelected | UIControlStateDisabled]
							key:@"title-color-selected-disabled"];

	[LinphoneUtils addDictEntry:attributes item:NSStringFromUIEdgeInsets([button titleEdgeInsets]) key:@"title-edge"];
	[LinphoneUtils addDictEntry:attributes
						   item:NSStringFromUIEdgeInsets([button contentEdgeInsets])
							key:@"content-edge"];
	[LinphoneUtils addDictEntry:attributes item:NSStringFromUIEdgeInsets([button imageEdgeInsets]) key:@"image-edge"];

	[LinphoneUtils addDictEntry:attributes item:[button imageForState:UIControlStateNormal] key:@"image-normal"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button imageForState:UIControlStateHighlighted]
							key:@"image-highlighted"];
	[LinphoneUtils addDictEntry:attributes item:[button imageForState:UIControlStateDisabled] key:@"image-disabled"];
	[LinphoneUtils addDictEntry:attributes item:[button imageForState:UIControlStateSelected] key:@"image-selected"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button imageForState:UIControlStateDisabled | UIControlStateHighlighted]
							key:@"image-disabled-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button imageForState:UIControlStateSelected | UIControlStateHighlighted]
							key:@"image-selected-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button imageForState:UIControlStateSelected | UIControlStateDisabled]
							key:@"image-selected-disabled"];

	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateNormal]
							key:@"background-normal"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateHighlighted]
							key:@"background-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateDisabled]
							key:@"background-disabled"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateSelected]
							key:@"background-selected"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateDisabled | UIControlStateHighlighted]
							key:@"background-disabled-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateSelected | UIControlStateHighlighted]
							key:@"background-selected-highlighted"];
	[LinphoneUtils addDictEntry:attributes
						   item:[button backgroundImageForState:UIControlStateSelected | UIControlStateDisabled]
							key:@"background-selected-disabled"];
}

+ (void)buttonMultiViewApplyAttributes:(NSDictionary *)attributes button:(UIButton *)button {
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-normal"] forState:UIControlStateNormal];
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-highlighted"]
			forState:UIControlStateHighlighted];
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-disabled"] forState:UIControlStateDisabled];
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-selected"] forState:UIControlStateSelected];
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-disabled-highlighted"]
			forState:UIControlStateDisabled | UIControlStateHighlighted];
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-selected-highlighted"]
			forState:UIControlStateSelected | UIControlStateHighlighted];
	[button setTitle:[LinphoneUtils getDictEntry:attributes key:@"title-selected-disabled"]
			forState:UIControlStateSelected | UIControlStateDisabled];

	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-normal"]
				 forState:UIControlStateNormal];
	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-highlighted"]
				 forState:UIControlStateHighlighted];
	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-disabled"]
				 forState:UIControlStateDisabled];
	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-selected"]
				 forState:UIControlStateSelected];
	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-disabled-highlighted"]
				 forState:UIControlStateDisabled | UIControlStateHighlighted];
	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-selected-highlighted"]
				 forState:UIControlStateSelected | UIControlStateHighlighted];
	[button setTitleColor:[LinphoneUtils getDictEntry:attributes key:@"title-color-selected-disabled"]
				 forState:UIControlStateSelected | UIControlStateDisabled];

	[button setTitleEdgeInsets:UIEdgeInsetsFromString([LinphoneUtils getDictEntry:attributes key:@"title-edge"])];
	[button setContentEdgeInsets:UIEdgeInsetsFromString([LinphoneUtils getDictEntry:attributes key:@"content-edge"])];
	[button setImageEdgeInsets:UIEdgeInsetsFromString([LinphoneUtils getDictEntry:attributes key:@"image-edge"])];

	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-normal"] forState:UIControlStateNormal];
	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-highlighted"]
			forState:UIControlStateHighlighted];
	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-disabled"] forState:UIControlStateDisabled];
	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-selected"] forState:UIControlStateSelected];
	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-disabled-highlighted"]
			forState:UIControlStateDisabled | UIControlStateHighlighted];
	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-selected-highlighted"]
			forState:UIControlStateSelected | UIControlStateHighlighted];
	[button setImage:[LinphoneUtils getDictEntry:attributes key:@"image-selected-disabled"]
			forState:UIControlStateSelected | UIControlStateDisabled];

	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-normal"]
					  forState:UIControlStateNormal];
	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-highlighted"]
					  forState:UIControlStateHighlighted];
	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-disabled"]
					  forState:UIControlStateDisabled];
	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-selected"]
					  forState:UIControlStateSelected];
	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-disabled-highlighted"]
					  forState:UIControlStateDisabled | UIControlStateHighlighted];
	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-selected-highlighted"]
					  forState:UIControlStateSelected | UIControlStateHighlighted];
	[button setBackgroundImage:[LinphoneUtils getDictEntry:attributes key:@"background-selected-disabled"]
					  forState:UIControlStateSelected | UIControlStateDisabled];
}

+ (void)addDictEntry:(NSMutableDictionary *)dict item:(id)item key:(id)key {
	if (item != nil && key != nil) {
		[dict setObject:item forKey:key];
	}
}

+ (id)getDictEntry:(NSDictionary *)dict key:(id)key {
	if (key != nil) {
		return [dict objectForKey:key];
	}
	return nil;
}

+ (NSString *)deviceModelIdentifier {
	struct utsname systemInfo;
	uname(&systemInfo);

	NSString *machine = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];

	if ([machine isEqual:@"iPad1,1"])
		return @"iPad";
	else if ([machine isEqual:@"iPad2,1"])
		return @"iPad 2";
	else if ([machine isEqual:@"iPad2,2"])
		return @"iPad 2";
	else if ([machine isEqual:@"iPad2,3"])
		return @"iPad 2";
	else if ([machine isEqual:@"iPad2,4"])
		return @"iPad 2";
	else if ([machine isEqual:@"iPad3,1"])
		return @"iPad 3";
	else if ([machine isEqual:@"iPad3,2"])
		return @"iPad 3";
	else if ([machine isEqual:@"iPad3,3"])
		return @"iPad 3";
	else if ([machine isEqual:@"iPad3,4"])
		return @"iPad 4";
	else if ([machine isEqual:@"iPad3,5"])
		return @"iPad 4";
	else if ([machine isEqual:@"iPad3,6"])
		return @"iPad 4";
	else if ([machine isEqual:@"iPad4,1"])
		return @"iPad Air";
	else if ([machine isEqual:@"iPad4,2"])
		return @"iPad Air";
	else if ([machine isEqual:@"iPad4,3"])
		return @"iPad Air";
	else if ([machine isEqual:@"iPad5,3"])
		return @"iPad Air 2";
	else if ([machine isEqual:@"iPad5,4"])
		return @"iPad Air 2";
	else if ([machine isEqual:@"iPad6,7"])
		return @"iPad Pro 12.9";
	else if ([machine isEqual:@"iPad6,8"])
		return @"iPad Pro 12.9";
	else if ([machine isEqual:@"iPad6,3"])
		return @"iPad Pro 9.7";
	else if ([machine isEqual:@"iPad6,4"])
		return @"iPad Pro 9.7";
	else if ([machine isEqual:@"iPad2,5"])
		return @"iPad mini";
	else if ([machine isEqual:@"iPad2,6"])
		return @"iPad mini";
	else if ([machine isEqual:@"iPad2,7"])
		return @"iPad mini";
	else if ([machine isEqual:@"iPad4,4"])
		return @"iPad mini 2";
	else if ([machine isEqual:@"iPad4,5"])
		return @"iPad mini 2";
	else if ([machine isEqual:@"iPad4,6"])
		return @"iPad mini 2";
	else if ([machine isEqual:@"iPad4,7"])
		return @"iPad mini 3";
	else if ([machine isEqual:@"iPad4,8"])
		return @"iPad mini 3";
	else if ([machine isEqual:@"iPad4,9"])
		return @"iPad mini 3";
	else if ([machine isEqual:@"iPad5,1"])
		return @"iPad mini 4";
	else if ([machine isEqual:@"iPad5,2"])
		return @"iPad mini 4";

	else if ([machine isEqual:@"iPhone1,1"])
		return @"iPhone";
	else if ([machine isEqual:@"iPhone1,2"])
		return @"iPhone 3G";
	else if ([machine isEqual:@"iPhone2,1"])
		return @"iPhone 3GS";
	else if ([machine isEqual:@"iPhone3,1"])
		return @"iPhone 4";
	else if ([machine isEqual:@"iPhone3,2"])
		return @"iPhone 4";
	else if ([machine isEqual:@"iPhone3,3"])
		return @"iPhone 4";
	else if ([machine isEqual:@"iPhone4,1"])
		return @"iPhone 4S";
	else if ([machine isEqual:@"iPhone5,1"])
		return @"iPhone5,2	iPhone 5";
	else if ([machine isEqual:@"iPhone5,3"])
		return @"iPhone5,4	iPhone 5c";
	else if ([machine isEqual:@"iPhone6,1"])
		return @"iPhone6,2	iPhone 5s";
	else if ([machine isEqual:@"iPhone7,2"])
		return @"iPhone 6";
	else if ([machine isEqual:@"iPhone7,1"])
		return @"iPhone 6 Plus";
	else if ([machine isEqual:@"iPhone8,1"])
		return @"iPhone 6s";
	else if ([machine isEqual:@"iPhone8,2"])
		return @"iPhone 6s Plus";
	else if ([machine isEqual:@"iPhone8,4"])
		return @"iPhone SE";

	else if ([machine isEqual:@"iPod1,1"])
		return @"iPod touch";
	else if ([machine isEqual:@"iPod2,1"])
		return @"iPod touch 2G";
	else if ([machine isEqual:@"iPod3,1"])
		return @"iPod touch 3G";
	else if ([machine isEqual:@"iPod4,1"])
		return @"iPod touch 4G";
	else if ([machine isEqual:@"iPod5,1"])
		return @"iPod touch 5G";
	else if ([machine isEqual:@"iPod7,1"])
		return @"iPod touch 6G";

	else if ([machine isEqual:@"x86_64"])
		return @"simulator 64bits";

	// none matched: cf https://www.theiphonewiki.com/wiki/Models for the whole list
	LOGW(@"%s: Oops, unknown machine %@... consider completing me!", __FUNCTION__, machine);
	return machine;
}

+ (LinphoneAddress *)normalizeSipOrPhoneAddress:(NSString *)value {
  	if (!value || [value isEqualToString:@""])
    	return NULL;

	LinphoneAccount *account = linphone_core_get_default_account(LC);
  	const char *normvalue;
	normvalue = linphone_account_is_phone_number(account, value.UTF8String)
	  	? linphone_account_normalize_phone_number(account, value.UTF8String)
		: value.UTF8String;

  	LinphoneAddress *addr = linphone_account_normalize_sip_uri(account, normvalue);
  	// first try to find a friend with the given address
  	Contact *c = [FastAddressBook getContactWithAddress:addr];

  	if (c && c.friend) {
    	LinphoneFriend *f = c.friend;
    	const LinphonePresenceModel *m = f
			? linphone_friend_get_presence_model_for_uri_or_tel(f, value.UTF8String)
			: NULL;
    	char *contact = m ? linphone_presence_model_get_contact(m) : NULL;
    	if (contact) {
      		LinphoneAddress *contact_addr = linphone_address_new(contact);
			ms_free(contact);
      		if (contact_addr) {
				linphone_address_unref(addr);
        		return contact_addr;
      		}
    	}
	}

	// since user wants to escape plus, we assume it expects to have phone
	// numbers by default
	if (addr && account) {
		const char *username = linphone_account_params_get_dial_escape_plus_enabled(linphone_account_get_params(account)) ? normvalue : value.UTF8String;
		if (linphone_account_is_phone_number(account, username))
			linphone_address_set_username(addr, linphone_account_normalize_phone_number(account, username));
	 }
	return addr;
}

+ (NSArray *)parseRecordingName:(NSString *)filename {
    NSString *rec = @"recording_"; //key that helps find recordings
    NSString *subName = [filename substringFromIndex:[filename rangeOfString:rec].location]; //We remove the parent folders if they exist in the filename
    NSArray *splitString = [subName componentsSeparatedByString:@"_"];
    //splitString: first element is the 'recording' prefix, last element is the date with the "E-d-MMM-yyyy-HH-mm-ss" format.
    NSString *name = [[splitString subarrayWithRange:NSMakeRange(1, [splitString count] -2)] componentsJoinedByString:@""];
    NSDateFormatter *format = [[NSDateFormatter alloc] init];
    [format setDateFormat:@"E-d-MMM-yyyy-HH-mm-ss"];
    NSString *dateWithMkv = [splitString objectAtIndex:[splitString count]-1]; //this will be in the form "E-d-MMM-yyyy-HH-mm-ss.mkv", we have to delete the extension
    NSDate *date = [format dateFromString:[dateWithMkv substringToIndex:[dateWithMkv length] - 4]];
    NSArray *res = [NSArray arrayWithObjects:name, date, nil];
    return res;
}

+ (UIAlertController *)networkErrorView:(NSString *)action {
    UIAlertController *errView =
    [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Network Error", nil)
                                        message:NSLocalizedString([@"There is no network connection available, enable WIFI or WWAN prior to " stringByAppendingString:action],nil)
                                 preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
                                                            style:UIAlertActionStyleDefault
                                                          handler:^(UIAlertAction *action){
                                                          }];
    
    [errView addAction:defaultAction];
    return errView;
}

@end

@implementation NSNumber (HumanReadableSize)

- (NSString *)toHumanReadableSize {
	float floatSize = [self floatValue];
	if (floatSize < 1023)
		return ([NSString stringWithFormat:@"%1.0f bytes", floatSize]);
	floatSize = floatSize / 1024;
	if (floatSize < 1023)
		return ([NSString stringWithFormat:@"%1.1f KB", floatSize]);
	floatSize = floatSize / 1024;
	if (floatSize < 1023)
		return ([NSString stringWithFormat:@"%1.1f MB", floatSize]);
	floatSize = floatSize / 1024;

	return ([NSString stringWithFormat:@"%1.1f GB", floatSize]);
}

@end

@implementation UIImage (systemIcons)

+ (UIImage *)imageFromSystemBarButton:(UIBarButtonSystemItem)systemItem :(UIColor *) color {
    // thanks to Renetik https://stackoverflow.com/a/49822488
    UIToolbar *bar = UIToolbar.new;
    UIBarButtonItem *buttonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:systemItem target:nil action:nil];
    [bar setItems:@[buttonItem] animated:NO];
    [bar snapshotViewAfterScreenUpdates:YES];
    for (UIView *view in [(id) buttonItem view].subviews)
        if ([view isKindOfClass:UIButton.class]) {
            UIImage *image = [((UIButton *) view).imageView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
            UIGraphicsBeginImageContextWithOptions(image.size, NO, image.scale);
            //[color set];
            [image drawInRect:CGRectMake(0, 0, image.size.width, image.size.height)];
            image = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();
            return image;
        }
    return nil;
}

@end



@implementation UIImageView (ImageWithTint)

- (void)setImageNamed:(NSString *)name tintColor:(UIColor *)color {
	self.image =  [[UIImage imageNamed:name] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
	self.tintColor = color;
}

- (void)setImageNamed:(NSString *)name tintColorLetter:(NSString *)letter {
	UIColor *color = [UIColor color:letter];
	self.image =  [[UIImage imageNamed:name] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
	self.tintColor = color;
}

@end

@implementation NSString (md5)

- (NSString *)md5 {
	const char *ptr = [self UTF8String];
	unsigned char md5Buffer[CC_MD5_DIGEST_LENGTH];
	CC_MD5(ptr, (unsigned int)strlen(ptr), md5Buffer);
	NSMutableString *output = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
	for (int i = 0; i < CC_MD5_DIGEST_LENGTH; i++) {
		[output appendFormat:@"%02x", md5Buffer[i]];
	}

	return output;
}

- (BOOL)containsSubstring:(NSString *)str {
	if (UIDevice.currentDevice.systemVersion.doubleValue >= 8.0) {
#pragma deploymate push "ignored-api-availability"
		return [self containsString:str];
#pragma deploymate pop
	}
	return ([self rangeOfString:str].location != NSNotFound);
}

@end

@implementation ContactDisplay

+ (void)setDisplayNameLabel:(UILabel *)label forContact:(Contact *)contact {
	label.text = [FastAddressBook displayNameForContact:contact];
#if 0
	NSString *lLastName = CFBridgingRelease(ABRecordCopyValue(contact, kABPersonLastNameProperty));
	NSString *lLocalizedLastName = [FastAddressBook localizedLabel:lLastName];
	if (lLocalizedLastName) {
		[label boldSubstring:lLocalizedLastName];
	}
#endif
}

+ (void)setDisplayNameLabel:(UILabel *)label forAddress:(const LinphoneAddress *)addr {
	Contact *contact = [FastAddressBook getContactWithAddress:addr];
	if (contact) {
		[ContactDisplay setDisplayNameLabel:label forContact:contact];
	} else {
		label.text = [FastAddressBook displayNameForAddress:addr];
	}
}

+ (void)setDisplayNameLabel:(UILabel *)label forAddress:(const LinphoneAddress *)addr withAddressLabel:(UILabel*)addressLabel{
	Contact *contact = [FastAddressBook getContactWithAddress:addr];
	NSString *tmpAddress = nil;
	char *uri = linphone_address_as_string_uri_only(addr);
	if (contact) {
		[ContactDisplay setDisplayNameLabel:label forContact:contact];
		tmpAddress = [NSString stringWithUTF8String:uri];
		addressLabel.hidden = FALSE;
	} else {
		label.text = [FastAddressBook displayNameForAddress:addr];
		if([LinphoneManager.instance lpConfigBoolForKey:@"display_phone_only" inSection:@"app"])
			addressLabel.hidden = TRUE;
		else
			tmpAddress = [NSString stringWithUTF8String:uri];
	}
	ms_free(uri);
	NSRange range = [tmpAddress rangeOfString:@";"];
	if (range.location != NSNotFound) {
		tmpAddress = [tmpAddress substringToIndex:range.location];
	}
	addressLabel.text = tmpAddress;
}


@end

@implementation UIImage (squareCrop)

- (UIImage *)squareCrop {
	// This calculates the crop area.

	size_t originalWidth = CGImageGetWidth(self.CGImage);
	size_t originalHeight = CGImageGetHeight(self.CGImage);

	size_t edge = MIN(originalWidth, originalHeight);

	float posX = (originalWidth - edge) / 2.0f;
	float posY = (originalHeight - edge) / 2.0f;

	CGRect rect = CGRectMake(posX, posY, edge, edge);

	// Create bitmap image from original image data,
	// using rectangle to specify desired crop area
	CGImageRef imageRef = CGImageCreateWithImageInRect([self CGImage], rect);
	UIImage *img = [UIImage imageWithCGImage:imageRef];
	CGImageRelease(imageRef);

	return img; /*
	 UIImage *ret = nil;



	 CGRect cropSquare = CGRectMake(posX, posY, edge, edge);

 //	CGImageRef imageRef = CGImageCreateWithImageInRect([self CGImage], cropSquare);
 //	ret = [UIImage imageWithCGImage:imageRef scale:self.scale orientation:self.imageOrientation];
 //
 //	CGImageRelease(imageRef);

	 CGImageRef imageRef = CGImageCreateWithImageInRect(self.CGImage, cropSquare);
	 ret = [UIImage imageWithCGImage:imageRef scale:self.scale orientation:self.imageOrientation];
	 CGImageRelease(imageRef);


	 return ret;*/
}

- (UIImage *)scaleToSize:(CGSize)size squared:(BOOL)squared {
	UIImage *scaledImage = self;
	if (squared) {
		//		scaledImage = [self squareCrop];
		size.width = size.height = MAX(size.width, size.height);
	}

	UIGraphicsBeginImageContext(size);

	[scaledImage drawInRect:CGRectMake(0, 0, size.width, size.height)];
	scaledImage = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();

	return scaledImage;
}

+ (UIImage *)resizeImage:(UIImage *)image withMaxWidth:(float)maxWidth andMaxHeight:(float)maxHeight {
    float actualHeight = image.size.height;
    float actualWidth = image.size.width;
    float imgRatio = actualWidth / actualHeight;
    float maxRatio = maxWidth / maxHeight;
    float compressionQuality = 1;
    if (actualHeight > maxHeight || actualWidth > maxWidth)
    {
        if (imgRatio < maxRatio) {
            imgRatio = maxHeight / actualHeight;
            actualWidth = imgRatio * actualWidth;
            actualHeight = maxHeight;
        } else if (imgRatio > maxRatio) {
            imgRatio = maxWidth / actualWidth;
            actualHeight = imgRatio * actualHeight;
            actualWidth = maxWidth;
        } else {
            actualHeight = maxHeight;
            actualWidth = maxWidth;
        }
    }
    CGRect rect = CGRectMake(0.0, 0.0, actualWidth, actualHeight);
    UIGraphicsBeginImageContext(rect.size);
    [image drawInRect:rect];
    UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
    NSData *imageData = UIImageJPEGRepresentation(img, compressionQuality);
    UIGraphicsEndImageContext();
    return [UIImage imageWithData:imageData];
}

@end

@implementation UIColor (LightAndDark)

- (UIColor *)lumColor:(float)mult {
	float hsbH, hsbS, hsbB;
	float rgbaR, rgbaG, rgbaB, rgbaA;

	// Get RGB
	CGColorRef cgColor = [self CGColor];
	CGColorSpaceRef cgColorSpace = CGColorGetColorSpace(cgColor);
	if (CGColorSpaceGetModel(cgColorSpace) != kCGColorSpaceModelRGB) {
		LOGW(@"Can't convert not RGB color");
		return self;
	} else {
		const CGFloat *colors = CGColorGetComponents(cgColor);
		rgbaR = colors[0];
		rgbaG = colors[1];
		rgbaB = colors[2];
		rgbaA = CGColorGetAlpha(cgColor);
	}

	RGB2HSL(rgbaR, rgbaG, rgbaB, &hsbH, &hsbS, &hsbB);

	hsbB = MIN(MAX(hsbB * mult, 0.0), 1.0);

	HSL2RGB(hsbH, hsbS, hsbB, &rgbaR, &rgbaG, &rgbaB);

	return [UIColor colorWithRed:rgbaR green:rgbaG blue:rgbaB alpha:rgbaA];
}

- (UIColor *)adjustHue:(float)hm saturation:(float)sm brightness:(float)bm alpha:(float)am {
	float hsbH, hsbS, hsbB;
	float rgbaR, rgbaG, rgbaB, rgbaA;

	// Get RGB
	CGColorRef cgColor = [self CGColor];
	CGColorSpaceRef cgColorSpace = CGColorGetColorSpace(cgColor);
	if (CGColorSpaceGetModel(cgColorSpace) != kCGColorSpaceModelRGB) {
		LOGW(@"Can't convert not RGB color");
		return self;
	} else {
		const CGFloat *colors = CGColorGetComponents(cgColor);
		rgbaR = colors[0];
		rgbaG = colors[1];
		rgbaB = colors[2];
		rgbaA = CGColorGetAlpha(cgColor);
	}

	RGB2HSL(rgbaR, rgbaG, rgbaB, &hsbH, &hsbS, &hsbB);

	hsbH = MIN(MAX(hsbH + hm, 0.0), 1.0);
	hsbS = MIN(MAX(hsbS + sm, 0.0), 1.0);
	hsbB = MIN(MAX(hsbB + bm, 0.0), 1.0);
	rgbaA = MIN(MAX(rgbaA + am, 0.0), 1.0);

	HSL2RGB(hsbH, hsbS, hsbB, &rgbaR, &rgbaG, &rgbaB);

	return [UIColor colorWithRed:rgbaR green:rgbaG blue:rgbaB alpha:rgbaA];
}

- (UIColor *)lighterColor {
	return [self lumColor:1.3];
}

- (UIColor *)darkerColor {
	return [self lumColor:0.75];
}


static NSMutableDictionary *letterColors = nil;

+(UIColor *)color:(NSString *)letter {
	if (letterColors == nil)
		letterColors  = [[NSMutableDictionary alloc] init];
	if (![letterColors objectForKey:letter]) {
		UIImage *colorImage = [UIImage imageWithContentsOfFile:[NSString stringWithFormat:@"%@/color_%@.png",[[NSBundle mainBundle] bundlePath],letter]];
		[letterColors setObject:[UIColor colorWithPatternImage:colorImage] forKey:letter];
	}
	return [letterColors objectForKey:letter];
}

@end

@implementation UIImage (ForceDecode)

+ (UIImage *)decodedImageWithImage:(UIImage *)image {
	CGImageRef imageRef = image.CGImage;
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = CGBitmapContextCreate(
		NULL, CGImageGetWidth(imageRef), CGImageGetHeight(imageRef), 8,
		// Just always return width * 4 will be enough
		CGImageGetWidth(imageRef) * 4,
		// System only supports RGB, set explicitly
		colorSpace,
		// Makes system don't need to do extra conversion when displayed.
		// NOTE: here we remove the alpha channel for performance. Most of the time, images loaded
		//       from the network are jpeg with no alpha channel. As a TODO, finding a way to detect
		//       if alpha channel is necessary would be nice.
		kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Little);
	CGColorSpaceRelease(colorSpace);
	if (!context)
		return nil;

	CGRect rect = (CGRect){CGPointZero, {CGImageGetWidth(imageRef), CGImageGetHeight(imageRef)}};
	CGContextDrawImage(context, rect, imageRef);
	CGImageRef decompressedImageRef = CGBitmapContextCreateImage(context);
	CGContextRelease(context);

	UIImage *decompressedImage =
		[[UIImage alloc] initWithCGImage:decompressedImageRef scale:image.scale orientation:image.imageOrientation];
	CGImageRelease(decompressedImageRef);
	return decompressedImage;
}

@end

@implementation UIImage (ResizeAndThumbnail)

+ (UIImage *)UIImageThumbnail:(UIImage *)image thumbSize:(CGFloat) tbSize {
    // Create a thumbnail version of the image for the event object.
    CGSize size = image.size;
    CGSize croppedSize;
    CGFloat offsetX = 0.0;
    CGFloat offsetY = 0.0;
    CGFloat actualTbSize = MAX(tbSize, MAX(size.height, size.width));
    // check the size of the image, we want to make it
    // a square with sides the size of the smallest end
    if (size.width > size.height) {
        offsetX = (size.height - size.width) / 2;
        croppedSize = CGSizeMake(size.height, size.height);
    } else {
        offsetY = (size.width - size.height) / 2;
        croppedSize = CGSizeMake(size.width, size.width);
    }
    
    // Crop the image before resize
    CGRect clippedRect = CGRectMake(offsetX * -1,
                                    offsetY * -1,
                                    croppedSize.width,
                                    croppedSize.height);
    CGImageRef imageRef = CGImageCreateWithImageInRect([image CGImage],
                                                       clippedRect);
    
    UIImage *cropped = [UIImage imageWithCGImage:imageRef];
    CGImageRelease(imageRef);
    // Done cropping
    
    // Resize the image
    CGRect rect = CGRectMake(0, 0, actualTbSize, actualTbSize);
    
    UIGraphicsBeginImageContext(rect.size);
    [cropped drawInRect:rect];
    UIImage *thumbnail = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    // Done Resizing
    
    return thumbnail;
}


+ (UIImage *)UIImageResize:(UIImage *)image toSize:(CGSize) newSize {
    CGImageRef newImage = [image CGImage];
    CGSize originalSize = [image size];
    float originalAspectRatio = originalSize.width / originalSize.height;
    // We resize in width and crop in height
    if (originalSize.width > newSize.width) {
        int height = newSize.width / originalAspectRatio;
        newImage = [UIImage resizeCGImage:newImage toWidth:newSize.width andHeight:height];
        originalSize.height = height;
    }
    CGRect cropRect = CGRectMake(0, 0, newSize.width, newSize.height);
    if (newSize.height < originalSize.height) cropRect.origin.y = (originalSize.height - newSize.height)/2;
    newImage = CGImageCreateWithImageInRect(newImage, cropRect);
    
    
    UIImage *cropped = [UIImage imageWithCGImage:newImage];
    CGImageRelease(newImage);
    return cropped;
}

+ (CGImageRef)resizeCGImage:(CGImageRef)image toWidth:(int)width andHeight:(int)height {
    // create context, keeping original image properties
    CGColorSpaceRef colorspace = CGImageGetColorSpace(image);
    CGContextRef context = CGBitmapContextCreate(NULL, width, height,
                                                 CGImageGetBitsPerComponent(image),
                                                 CGImageGetBytesPerRow(image),
                                                 colorspace,
                                                 CGImageGetAlphaInfo(image));
    CGColorSpaceRelease(colorspace);
    
    if(context == NULL)
        return nil;
    
    // draw image to context (resizing it)
    CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
    // extract resulting image from context
    CGImageRef imgRef = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    
    return imgRef;
}


@end
