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

#import <QuartzCore/QuartzCore.h>

#import "UICallCell.h"

#import "LinphoneManager.h"
#import "FastAddressBook.h"

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
@synthesize headerBackgroundHighlightImage;

@synthesize addressLabel;
@synthesize stateLabel;
@synthesize stateImage;
@synthesize avatarImage;
@synthesize pauseButton;
@synthesize removeButton;

@synthesize headerView;
@synthesize avatarView;

@synthesize firstCell;
@synthesize conferenceCell;
@synthesize currentCall;


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
        
        self->currentCall = FALSE;
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
    [data release];
    [super dealloc];
}


- (void)prepareForReuse {
    [super prepareForReuse];
    self->currentCall = FALSE;
    [headerBackgroundHighlightImage setAlpha:0.0f];
    [data release];
    data = nil;
}


#pragma mark - Properties Functions

- (void)setData:(UICallCellData *)adata {
    if(data !=nil) {
        [data release];
        data = nil;
    }
    if(adata !=nil) {
        data = [adata retain];
    }
    [self update];
}

- (void)setCurrentCall:(BOOL) val {
    BOOL oldVal = currentCall;
    currentCall = val;
    if(oldVal != val) {
        if (currentCall) {
            [self startBlinkAnimation:@"Blink" target:headerBackgroundHighlightImage];
        } else {
            [self stopBlinkAnimation:@"Blink" target:headerBackgroundHighlightImage];
        }
    }
}


#pragma mark - Static Functions

+ (int)getMaximizedHeight {
    return 280;
}

+ (int)getMinimizedHeight {
    return 63; //MODIFICATION Change row behaviour
}

- (void)startBlinkAnimation:(NSString *)animationID  target:(UIView *)target {   
    [UIView animateWithDuration:1.0
                          delay: 0.0
                        options: UIViewAnimationOptionRepeat | 
                                 UIViewAnimationOptionAutoreverse | 
                                 UIViewAnimationOptionAllowUserInteraction | 
                                 UIViewAnimationOptionCurveEaseIn
                     animations:^{
                             [target setAlpha:1.0f];
                     }
                     completion:^(BOOL finished){
                     }];

}

- (void)stopBlinkAnimation:(NSString *)animationID target:(UIView *)target {
    [target.layer removeAnimationForKey:animationID];
    [target setAlpha:0.0f];
}

         
#pragma mark - 

- (void)update {
    if(data == nil || data->call == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update call cell: null call or data"];
        return;
    }
    LinphoneCall *call = data->call;
    const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    
    UIImage *image = nil;
    NSString* address  = nil;
    if(addr != NULL) {
        BOOL useLinphoneAddress = true;
        // contact name 
        const char* lAddress = linphone_address_as_string_uri_only(addr);
        if(lAddress) {
            NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:lAddress]];
            ABRecordRef contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
            if(contact) {
                image = [FastAddressBook getContactImage:contact thumbnail:false];
                address = [FastAddressBook getContactDisplayName:contact];
                useLinphoneAddress = false;
            }
        }
        if(useLinphoneAddress) {
            const char* lDisplayName = linphone_address_get_display_name(addr);
            const char* lUserName = linphone_address_get_username(addr);
            if (lDisplayName) 
                address = [NSString stringWithUTF8String:lDisplayName];
            else if(lUserName) 
                address = [NSString stringWithUTF8String:lUserName];
        }
    }
    
    // Set Image
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown.png"];
    }
    [avatarImage setImage:image];
    
    // Set Address
    if(address == nil) {
        address = @"Unknown";
    }
    [addressLabel setText:address];
    
    LinphoneCallState state = linphone_call_get_state(call);
    if(!conferenceCell) {
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
            [headerBackgroundHighlightImage setImage:[UIImage imageNamed:@"cell_call_first_highlight.png"]];
        } else {
            [headerBackgroundImage setImage:[UIImage imageNamed:@"cell_call.png"]];
            [headerBackgroundHighlightImage setImage:[UIImage imageNamed:@"cell_call_highlight.png"]];
        }
    } else {
        [stateImage setHidden:true];
        [pauseButton setHidden:true];
        [removeButton setHidden:false];
        [headerBackgroundImage setImage:[UIImage imageNamed:@"cell_conference.png"]];
    }
    
    int duration = linphone_call_get_duration(call);
    [stateLabel setText:[NSString stringWithFormat:@"%02i:%02i", (duration/60), duration - 60 * (duration / 60), nil]];
    
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
    [pauseButton setType:UIPauseButtonType_Call call:call];
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
