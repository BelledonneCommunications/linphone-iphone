/* UIDigitButton.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import "UIDigitButtonLongVoiceMail.h"
#import "Utils.h"
#include "LinphoneManager.h"

@implementation UIDigitButtonLongVoiceMail

#pragma mark - UILongTouchButtonDelegate Functions

- (void)onRepeatTouch {
}

- (void)onLongTouch {
	if ([self voiceMailEnabled]) {
		LinphoneManager *lm = [LinphoneManager instance];
		[lm call:[lm lpConfigStringForKey:@"voice_mail_uri"] displayName:NSLocalizedString(@"Voice mail",nil) transfer:FALSE];
	}
}

- (BOOL) voiceMailEnabled {
	NSString * voiceMailUri = [[LinphoneManager instance] lpConfigStringForKey:@"voice_mail_uri" withDefault:NULL];

	return (voiceMailUri != NULL);
}

- (void)refreshUI {
	NSString *name = @"numpad_one_";

	if ([self voiceMailEnabled]) {
		name = [name stringByAppendingString:@"voicemail_"];
	}

	[self setImage:[UIImage imageNamed:[name stringByAppendingString:@"default.png"]] forState: UIControlStateNormal];
	[self setImage:[UIImage imageNamed:[name stringByAppendingString:@"over.png"]] forState: UIControlStateHighlighted];
}

@end
