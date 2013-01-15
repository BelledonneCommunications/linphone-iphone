/* UILinphone.h
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

#import <UIKit/UIColor.h>

#define LINPHONE_MAIN_COLOR [UIColor colorWithRed:207.0f/255.0f green:76.0f/255.0f blue:41.0f/255.0f alpha:1.0f]
#define LINPHONE_TABLE_CELL_BACKGROUND_COLOR [UIColor colorWithRed:207.0f/255.0f green:76.0f/255.0f blue:41.0f/255.0f alpha:1.0f]

@interface UIColor (LightAndDark)

- (UIColor *)adjustHue:(float)hm saturation:(float)sm brightness:(float)bm alpha:(float)am;

- (UIColor *)lumColor:(float)mult;

- (UIColor *)lighterColor;

- (UIColor *)darkerColor;

@end


@interface UIImage (ForceDecode)

+ (UIImage *)decodedImageWithImage:(UIImage *)image;

@end
