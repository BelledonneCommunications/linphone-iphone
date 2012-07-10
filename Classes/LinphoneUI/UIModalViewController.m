/* UIModalViewController.h
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

#import "UIModalViewController.h"

@implementation UIModalViewController

- (void)initUIModalViewController {
    dismissed = FALSE;
}

- (id)init {
    self = [super init];
    if (self) {
		[self initUIModalViewController];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self initUIModalViewController];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUIModalViewController];
	}
    return self;
}	

- (void)dismiss:(id)value {
    if(modalDelegate != nil)
        [modalDelegate modalViewDismiss:self value:value];
    
    if(!dismissed) {
        dismissed = true;
        [self autorelease];
    }
}

- (void)dismiss{
    if(modalDelegate != nil)
        [modalDelegate modalViewDismiss:self value:nil];
    
    if(!dismissed) {
        dismissed = true;
        [self autorelease];
    }
}

- (void)setModalDelegate:(id<UIModalViewDelegate>)delegate {
    modalDelegate = delegate;   
}

@end
