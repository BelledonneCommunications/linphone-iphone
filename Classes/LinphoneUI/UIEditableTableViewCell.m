/* UIEditableTableViewCell.m
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

#import "UIEditableTableViewCell.h"

@implementation UIEditableTableViewCell

@synthesize detailTextField;


#pragma mark - Lifecycle Functions

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        UIView *parent = [self.detailTextLabel superview];
        self.detailTextField = [[UITextField alloc] init];
        [self.detailTextField setHidden:TRUE];
        [self.detailTextField setClearButtonMode: UITextFieldViewModeWhileEditing];
        [self.detailTextField setContentVerticalAlignment: UIControlContentVerticalAlignmentCenter];

        UIFont *font = [UIFont fontWithName:@"Helvetica-Bold" size:[UIFont systemFontSize]];
        [self.detailTextLabel setFont:font];
        [self.detailTextField setFont:font];
        [parent addSubview:detailTextField];
    }
    return self;
}

- (void)dealloc {
    [self.detailTextField release];
    [super dealloc];
}


#pragma mark - View Functions

- (void)layoutSubviews {
    [super layoutSubviews];	
    
    CGRect fieldframe;
    fieldframe.origin.x = 15;
    fieldframe.origin.y = 0;
    fieldframe.size.height = 44;
    if([[self.textLabel text] length] != 0)
        fieldframe.origin.x += [self.textLabel frame].size.width;
    CGRect superframe = [[self.detailTextField superview]frame];
    fieldframe.size.width = superframe.size.width - fieldframe.origin.x;
    [self.detailTextField setFrame:fieldframe];
    
    CGRect labelFrame = [self.detailTextLabel frame];
    labelFrame.origin.x = fieldframe.origin.x;
    [self.detailTextLabel setFrame:labelFrame];
}


#pragma mark - UITableViewCell Functions

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    [super setEditing:editing animated:animated];
    if(editing) {
        [self.detailTextField setHidden:FALSE];
        [self.detailTextLabel setHidden:TRUE];
    } else {
        [self.detailTextField setHidden:TRUE];
        [self.detailTextLabel setHidden:FALSE];
    }
}

- (void)setEditing:(BOOL)editing {
    [self setEditing:editing animated:FALSE];
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
    [super setSelected:selected animated:animated];
}

@end
