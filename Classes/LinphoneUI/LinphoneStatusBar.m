/* StatusSubViewController.h
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */     

#import "LinphoneStatusBar.h"

@implementation LinphoneStatusBar

@synthesize image;
@synthesize spinner;
@synthesize label;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(registrationStateChange:) name:@"LinphoneRegistrationStateChange" object:nil];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void) registrationStateChange: (NSNotification*) notif {  
    LinphoneRegistrationState state = [[notif.userInfo objectForKey: @"state"] intValue];
    NSString* message = [notif.userInfo objectForKey: @"message"];
    label.hidden = NO;
    switch(state) {
        case LinphoneRegistrationCleared:
/*            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_orange.png"]];
            [spinner stopAnimating];
            [label setText:message != nil ? message : NSLocalizedString(@"No SIP account defined", nil)];*/
        case LinphoneRegistrationFailed:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_red.png"]];
            [spinner stopAnimating];
            [label setText:message];
        case LinphoneRegistrationNone:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_gray.png"]];
            [spinner stopAnimating];
            [label setText:message];
        case LinphoneRegistrationProgress:
            image.hidden = YES;
            spinner.hidden = NO;
            [spinner startAnimating];
            [label setText:message];
        case LinphoneRegistrationOk:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_green.png"]];
            [spinner stopAnimating];
            [label setText:message];
    }
}

- (void) viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) dealloc {
    [super dealloc];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
