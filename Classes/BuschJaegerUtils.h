/* BuschJaegerUtils.h
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

#import <Foundation/Foundation.h>

#define BUSCHJAEGER_NORMAL_COLOR [UIColor colorWithRed:32.0/255 green:45.0/255 blue:62.0/255 alpha:1.0]
#define BUSCHJAEGER_NORMAL_COLOR2 [UIColor colorWithRed:18.0/255 green:26.0/255 blue:41.0/255 alpha:1.0]

#define BUSCHJAEGER_RED_COLOR [UIColor colorWithRed:153.0/255 green:48.0/255 blue:48.0/255 alpha:1.0]
#define BUSCHJAEGER_RED_COLOR2 [UIColor colorWithRed:66.0/255 green:15.0/255 blue:15.0/255 alpha:1.0]

#define BUSCHJAEGER_GREEN_COLOR [UIColor colorWithRed:91.0/255 green:161.0/255 blue:89.0/255 alpha:1.0]
#define BUSCHJAEGER_GREEN_COLOR2 [UIColor colorWithRed:25.0/255 green:54.0/255 blue:24.0/255 alpha:1.0]

#define BUSCHJAEGER_GRAY_COLOR [UIColor colorWithRed:161.0/255 green:161.0/255 blue:161.0/255 alpha:1.0]
#define BUSCHJAEGER_GRAY_COLOR2 [UIColor colorWithRed:54.0/255 green:54.0/255 blue:54.0/255 alpha:1.0]

#define BUSCHJAEGER_DEFAULT_CORNER_RADIUS 5

@interface BuschJaegerUtils : NSObject

+ (void)createGradientForView:(UIView*)view withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor;
+ (void)createGradientForButton:(UIButton*)button withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor;
+ (void)createGradientForView:(UIView*)view withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor cornerRadius:(int)cornerRadius;
+ (void)createGradientForButton:(UIButton*)button withTopColor:(UIColor*)topColor bottomColor:(UIColor*)bottomColor cornerRadius:(int)cornerRadius;

@end
