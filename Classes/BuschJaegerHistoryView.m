/* BuschJaegerHistoryView.h
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


#import "BuschJaegerHistoryView.h"
#import "BuschJaegerMainView.h"
#import "BuschJaegerUtils.h"

@implementation BuschJaegerHistoryView

@synthesize backButton;
@synthesize waitView;
@synthesize tableController;


#pragma mark - Lifecycle Functions 

- (void)dealloc {
    [backButton release];
    [waitView release];
    [tableController release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [tableController.view setBackgroundColor:[UIColor clearColor]];
    
    [waitView setHidden:TRUE];
    
    /* init gradients */
    {
        UIColor* col1 = BUSCHJAEGER_NORMAL_COLOR;
        UIColor* col2 = BUSCHJAEGER_NORMAL_COLOR2;
        
        [BuschJaegerUtils createGradientForView:backButton withTopColor:col1 bottomColor:col2 cornerRadius:BUSCHJAEGER_DEFAULT_CORNER_RADIUS];
    }
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if ([LinphoneManager runningOnIpad]) {
        return YES;
    } else {
        return [super shouldAutorotateToInterfaceOrientation:interfaceOrientation];
    }
}

- (NSUInteger)supportedInterfaceOrientations {
    if ([LinphoneManager runningOnIpad]) {
        return UIInterfaceOrientationMaskAll;
    } else {
        return [super supportedInterfaceOrientations];
    }
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)sender {
    [[BuschJaegerMainView instance].navigationController popViewControllerAnimated:FALSE];
}


#pragma mark - 

- (void)reload {
    [self view]; // Force view load
    if([[LinphoneManager instance].configuration loadHistory:self]) {
        [waitView setHidden:FALSE];
    } else {
        [waitView setHidden:TRUE];
    }
}

- (void)update {
    NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"date" ascending:NO];
    NSArray *sortDescriptors = [NSArray arrayWithObjects:sortDescriptor, nil];
    
    [tableController setHistory:[[LinphoneManager instance].configuration.history sortedArrayUsingDescriptors:sortDescriptors]];
}


#pragma mark - BuschJaegerConfigurationDelegate Functions

- (void)buschJaegerConfigurationSuccess {
    [waitView setHidden:TRUE];
    [self update];
}

- (void)buschJaegerConfigurationError:(NSString *)error {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"History provisioning error",nil)
                                                        message:[NSString stringWithFormat:NSLocalizedString(@"Connection issue: %@", nil), error]
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    [waitView setHidden:TRUE];
    [self update];
}


@end
