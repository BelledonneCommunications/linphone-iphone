/* UIView+ModalStack.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import <QuartzCore/QuartzCore.h>

#import "UIView+ModalStack.h"

@implementation UIView (ModalStack)

- (void)addModalView:(UIView*)view {
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionFade];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:kCATransitionFromRight];
    [self.layer addAnimation:trans forKey:@"Appear"];
    
    [self addSubview:view];
    [self bringSubviewToFront:view];
}

- (void)removeModalView:(UIView*)view {
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionFade];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:kCATransitionFromRight];
    [self.layer addAnimation:trans forKey:@"Disappear"];
    
    [view removeFromSuperview];
}

@end
