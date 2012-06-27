/* UICallCell.m
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

#import "UICallCell.h"

@implementation UICallCellData

- (id)init:(LinphoneCall*) acall {
    self = [super init];
    if(self != nil) {
        self->minimize = false;
        self->call = acall;
    }
    return self;
}
@end

@implementation UICallCell

@synthesize data;

@synthesize firstBackground;
@synthesize otherBackground;

@synthesize addressLabel;
@synthesize stateLabel;
@synthesize stateImage;
@synthesize avatarImage;

@synthesize headerView;
@synthesize avatarView;

- (id)init {
    if ((self = [super init]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UICallCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
    }
    return self;
}

- (void)firstCell{
    [firstBackground setHidden:false];
    [otherBackground setHidden:true];
}

- (void)otherCell{
    [firstBackground setHidden:true];
    [otherBackground setHidden:false];
}

- (void)update:(UICallCellData*) adata {
    self->data = adata;
    [self update];
}

- (void)update {
    if(data) {
        LinphoneCall *call = data->call;
        const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    
        if (addr) {
            const char* lUserNameChars=linphone_address_get_username(addr);
            NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
            NSMutableString* mss = [[NSMutableString alloc] init];
            // contact name 
            const char* n = linphone_address_get_display_name(addr);
            if (n) 
                [mss appendFormat:@"%s", n, nil];
            else
                [mss appendFormat:@"%@",lUserName , nil];
        
            [addressLabel setText:mss];
        
            // TODO
            //imageView.image = [[LinphoneManager instance] getImageFromAddressBook:lUserName];
            [mss release];
        } else {
            [addressLabel setText:@"Unknown"];
            //TODO
            //imageView.image = nil;
        }
    
    
        LinphoneCallState state = linphone_call_get_state(call);
        if(state == LinphoneCallPaused || state == LinphoneCallPausing) {
            [stateImage setImage:[UIImage imageNamed:@"pause-champ-numero-actif"]];
        } else if(state == LinphoneCallOutgoingRinging) {
            [stateImage setImage:[UIImage imageNamed:@"ring-champ-numero-actif"]];
        } else if(state == LinphoneCallOutgoingInit || state == LinphoneCallOutgoingProgress){
            [stateImage setImage:nil];
        } else {
            [stateImage setImage:[UIImage imageNamed:@"play-champ-numero-actif"]];
        }
    
        NSMutableString* msDuration = [[NSMutableString alloc] init];
        int duration = linphone_call_get_duration(call);
        [msDuration appendFormat:@"%02i:%02i", (duration/60), duration - 60 * (duration / 60), nil];
        [stateLabel setText:msDuration];
        [msDuration release];
        
        if(!data->minimize) {
            CGRect frame = [self frame];
            frame.size.height = [avatarView frame].size.height;
            [self setFrame:frame];
            [avatarView setHidden:false];
        } else {
            CGRect frame = [self frame];
            frame.size.height = [headerView frame].size.height;
            [self setFrame:frame];
            [avatarView setHidden:true];
        }
    }
}

- (IBAction)doHeaderClick:(id)sender {
    NSLog(@"Toggle UICallCell");
    if(data) {
        data->minimize = !data->minimize;
        [self selfUpdate];
    }
}

- (void)selfUpdate {
    UITableView *parentTable = (UITableView *)self.superview;
    if(parentTable) {
       NSIndexPath *index= [parentTable indexPathForCell:self];
        if(index != nil) {
            [parentTable reloadRowsAtIndexPaths:[[NSArray alloc] initWithObjects:index, nil] withRowAnimation:false];
        }
    }
}

- (void)dealloc {
    [firstBackground release];
    [otherBackground release];
    [addressLabel release];
    [stateLabel release];
    [stateImage release];
    [avatarImage release];
    [headerView release];
    [super dealloc];
}

+ (int)getMaximizedHeight {
    return 300;
}

+ (int)getMinimizedHeight {
    return 58;
}


@end
