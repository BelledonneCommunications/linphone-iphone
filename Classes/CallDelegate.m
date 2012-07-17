/* LinphoneManager.h
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

#import "CallDelegate.h"
#import "Utils.h"

@implementation CallDelegate

@synthesize eventType;
@synthesize call;
@synthesize delegate;
@synthesize timeout;

-(void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (timeout) {
        [timeout invalidate];
        timeout = nil;
    }
    if (eventType == CD_UNDEFINED) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Incorrect usage of CallDelegate/ActionSheet: eventType must be set"];
    }
    [delegate actionSheet:actionSheet ofType:eventType clickedButtonAtIndex:buttonIndex withUserDatas:call];
}

-(void) actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex {
	
		
    if (timeout) {
        [timeout invalidate];
        timeout = nil;
    }
	if (buttonIndex != actionSheet.cancelButtonIndex) return;
	
    if (eventType == CD_UNDEFINED) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Incorrect usage of CallDelegate/ActionSheet: eventType must be set"];
    }
	
    [delegate actionSheet:actionSheet ofType:eventType clickedButtonAtIndex:buttonIndex withUserDatas:call];    
}

-(void) actionSheetCancel:(UIActionSheet *)actionSheet {
    if (timeout) {
        [timeout invalidate];
        timeout = nil;
    }
    if (eventType == CD_UNDEFINED) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Incorrect usage of CallDelegate/ActionSheet: eventType must be set"];
    }
    [delegate actionSheet:actionSheet ofType:eventType clickedButtonAtIndex:actionSheet.cancelButtonIndex withUserDatas:call];
}

@end
