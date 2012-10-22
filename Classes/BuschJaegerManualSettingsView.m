/* BuschJagerManualSettingsView.m
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

#import "BuschJaegerManualSettingsView.h"
#import "BuschJaegerUtils.h"
#import "BuschJaegerMainView.h"

@implementation BuschJaegerManualSettingsView

@synthesize addressField;
@synthesize usernameField;
@synthesize passwordField;

@synthesize validButton;
@synthesize backButton;

#pragma mark - Lifecycle Functions

- (void)initBuschJaegerManualSettingsView {
}

- (id)init {
    self = [super init];
    if(self != nil) {
        [self initBuschJaegerManualSettingsView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if(self != nil) {
        [self initBuschJaegerManualSettingsView];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if(self != nil) {
        [self initBuschJaegerManualSettingsView];
    }
    return self;
}

- (void)dealloc {
    [addressField release];
    [usernameField release];
    [passwordField release];
    
    [validButton release];
    [backButton release];
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    /* init gradients */
    {
        UIColor* col1 = BUSCHJAEGER_NORMAL_COLOR;
        UIColor* col2 = BUSCHJAEGER_NORMAL_COLOR2;
        
        [BuschJaegerUtils createGradientForView:validButton withTopColor:col1 bottomColor:col2 cornerRadius:BUSCHJAEGER_DEFAULT_CORNER_RADIUS];
        [BuschJaegerUtils createGradientForView:backButton withTopColor:col1 bottomColor:col2 cornerRadius:BUSCHJAEGER_DEFAULT_CORNER_RADIUS];
    }
}


#pragma mark - Action Functions

- (IBAction)onValidClick:(id)sender {
    if([[addressField text] length] && [[usernameField text] length] && [[passwordField text] length]) {
        [[BuschJaegerMainView instance].navigationController popViewControllerAnimated:FALSE];
        [[BuschJaegerMainView instance].settingsView setConfiguration:[addressField text] username:[usernameField text] password:[passwordField text]];
    }
}

- (IBAction)onBackClick:(id)sender {
    [[BuschJaegerMainView instance].navigationController popViewControllerAnimated:FALSE];
}


#pragma mark - 

- (void)reset {
    [addressField setText:@""];
    [usernameField setText:@""];
    [passwordField setText:@""];
    [addressField resignFirstResponder];
    [usernameField resignFirstResponder];
    [passwordField resignFirstResponder];
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return TRUE;
}

@end
