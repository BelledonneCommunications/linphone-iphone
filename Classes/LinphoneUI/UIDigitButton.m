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

@synthesize sendDtmfDuringCall;



-(void) touchDown:(id) sender {
    linphone_core_send_dtmf([LinphoneManager getLc],mDigit);
}

-(void) touchUp:(id) sender {
	linphone_core_stop_dtmf([LinphoneManager getLc]);
	if (mDigit == '0') {
		//cancel timer for +
		[NSObject cancelPreviousPerformRequestsWithTarget:self 
												 selector:@selector(doKeyZeroLongPress)
												   object:nil];
	} 
	
	
}

-(void)doKeyZeroLongPress {
	NSString* newAddress = [[mAddress.text substringToIndex: [mAddress.text length]-1]  stringByAppendingString:@"+"];
	[mAddress setText:newAddress];
	
}

-(void) initWithNumber:(char)digit {
	[self initWithNumber:digit addressField:nil dtmf:true];
}
-(void) initWithNumber:(char)digit  addressField:(UITextField*) address dtmf:(bool_t)sendDtmf{
    sendDtmfDuringCall = sendDtmf;
	mDigit=digit ;
	mAddress=address?[address retain]:nil;
	[self addTarget:self action:@selector(touchDown:) forControlEvents:UIControlEventTouchDown];
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside];
}
/*
 // Only override drawRect: if you perform custom drawing.
 // An empty implementation adversely affects performance during animation.
 - (void)drawRect:(CGRect)rect {
 // Drawing code.
 }
 */

- (void)dealloc {
    [super dealloc];
	[mAddress release];
}


@end
