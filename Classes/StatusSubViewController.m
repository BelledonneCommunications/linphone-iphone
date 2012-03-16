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

#import "StatusSubViewController.h"

@implementation StatusSubViewController

@synthesize image;
@synthesize spinner;
@synthesize label;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

-(BOOL) updateWithRegistrationState:(LinphoneRegistrationState)state message:(NSString*) message {
    label.hidden = NO;
    switch(state) {
        case LinphoneRegistrationCleared:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_orange.png"]];
            [spinner stopAnimating];
            [label setText:message != nil ? message : NSLocalizedString(@"No SIP account defined", nil)];
            return YES;
        case LinphoneRegistrationFailed:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_red.png"]];
            [spinner stopAnimating];
            [label setText:message];
            return NO;
        case LinphoneRegistrationNone:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_gray.png"]];
            [spinner stopAnimating];
            [label setText:message];
            return NO;
        case LinphoneRegistrationProgress:
            image.hidden = YES;
            spinner.hidden = NO;
            [spinner startAnimating];
            [label setText:message];
            return NO;
        case LinphoneRegistrationOk:
            image.hidden = NO;
            [image setImage:[UIImage imageNamed:@"status_green.png"]];
            [spinner stopAnimating];
            [label setText:message];
            return YES;
    }
    return NO;
}

@end
