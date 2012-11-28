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

#import "UIDigitButton.h"
#include "linphonecore.h"
#import "LinphoneManager.h"

@implementation UIDigitButton

@synthesize dtmf;
@synthesize digit;
@synthesize addressField;


#pragma mark - Lifecycle Functions

- (void)initUIDigitButton {
    dtmf = FALSE;
	[self addTarget:self action:@selector(touchDown:) forControlEvents:UIControlEventTouchDown];
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside];
}

- (id)init {
    self = [super init];
    if (self) {
		[self initUIDigitButton];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
		[self initUIDigitButton];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUIDigitButton];
	}
    return self;
}	

- (void)dealloc {
	[addressField release];
    [super dealloc];
}


#pragma mark - Actions Functions

- (void)touchDown:(id) sender {
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot trigger digit button: Linphone core not ready"];
        return;
    }
	if (addressField && (!dtmf || !linphone_core_in_call([LinphoneManager getLc]))) {
		NSString* newAddress = [NSString stringWithFormat:@"%@%c",addressField.text, digit];
		[addressField setText:newAddress];
		linphone_core_play_dtmf([LinphoneManager getLc], digit, -1);
	} else {
		linphone_core_send_dtmf([LinphoneManager getLc], digit);
		linphone_core_play_dtmf([LinphoneManager getLc], digit, 100);
	}
}

- (void)touchUp:(id) sender {
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot trigger digit button: Linphone core not ready"];
        return;
    }
	linphone_core_stop_dtmf([LinphoneManager getLc]);
}


#pragma mark - UILongTouchButtonDelegate Functions

- (void)onRepeatTouch {
}

- (void)onLongTouch {
    if (digit == '0') {
        NSString* newAddress = [[addressField.text substringToIndex: [addressField.text length]-1]  stringByAppendingString:@"+"];
        [addressField setText:newAddress];
    }
}

@end
