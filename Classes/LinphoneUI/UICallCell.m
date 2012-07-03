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

#import "LinphoneManager.h"

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

@synthesize headerBackgroundImage;

@synthesize addressLabel;
@synthesize stateLabel;
@synthesize stateImage;
@synthesize avatarImage;
@synthesize pauseButton;
@synthesize removeButton;

@synthesize headerView;
@synthesize avatarView;

@synthesize firstCell;
@synthesize conferenceCall;


#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UICallCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
        // Set selected+over background: IB lack !
        [pauseButton setImage:[UIImage imageNamed:@"call_state_pause_over.png"] 
                              forState:(UIControlStateHighlighted | UIControlStateSelected)];
    }
    return self;
}

- (void)dealloc {
    [headerBackgroundImage release];
    [addressLabel release];
    [stateLabel release];
    [stateImage release];
    [avatarImage release];
    [headerView release];
    [super dealloc];
}


#pragma mark - Static cell sizes

+ (int)getMaximizedHeight {
    return 280;
}

+ (int)getMinimizedHeight {
    return 54;
}


#pragma mark - 

- (void)update:(UICallCellData*) adata {
    self->data = adata;
    [self update];
}

- (void)update {
    LinphoneCall *call = NULL;
    if(data != nil && data->call != NULL) {
        call = data->call;
        const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    
        if (addr != NULL) {
            const char* lUserNameChars = linphone_address_get_username(addr);
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
        
        if(!conferenceCall) {
            if(state == LinphoneCallOutgoingRinging) {
                [stateImage setImage:[UIImage imageNamed:@"call_state_ringing_default.png"]];
                [stateImage setHidden:false];
                [pauseButton setHidden:true];
            } else if(state == LinphoneCallOutgoingInit || state == LinphoneCallOutgoingProgress){
                [stateImage setImage:[UIImage imageNamed:@"call_state_outgoing_default.png"]];
                [stateImage setHidden:false];
                [pauseButton setHidden:true];
            } else {
                [stateImage setHidden:true];
                [pauseButton setHidden:false];
                [pauseButton update];
            }
            [removeButton setHidden:true];
            if(firstCell) {
                [headerBackgroundImage setImage:[UIImage imageNamed:@"cell_call_first.png"]];
            } else {
                [headerBackgroundImage setImage:[UIImage imageNamed:@"cell_call.png"]];
            }
        } else {
            [stateImage setHidden:true];
            [pauseButton setHidden:true];
            [removeButton setHidden:false];
            [headerBackgroundImage setImage:[UIImage imageNamed:@"cell_conference.png"]];
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
    [pauseButton setType:UIPauseButtonType_Call call:call];
}

- (void)selfUpdate {
    UITableView *parentTable = (UITableView *)self.superview;
    [parentTable beginUpdates];
    [parentTable reloadData];
    [parentTable endUpdates];
    /*
    if(parentTable) {
       NSIndexPath *index= [parentTable indexPathForCell:self];
        if(index != nil) {
            [parentTable reloadRowsAtIndexPaths:[[NSArray alloc] initWithObjects:index, nil] withRowAnimation:false];
        }
    }*/
}


#pragma mark - Action Functions

- (IBAction)doHeaderClick:(id)sender {
    if(data) {
        data->minimize = !data->minimize;
        [self selfUpdate];
    }
}

- (IBAction)doRemoveClick:(id)sender {
    if(data != nil && data->call != NULL) {
        linphone_core_remove_from_conference([LinphoneManager getLc], data->call);
    }
}

@end
