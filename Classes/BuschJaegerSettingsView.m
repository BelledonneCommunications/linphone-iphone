/* BuschJaegerSettingsView.m
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

#import "BuschJaegerSettingsView.h"
#import "BuschJaegerUtils.h"
#import "BuschJaegerMainView.h"

@implementation BuschJaegerSettingsView

@synthesize scanButton;
@synthesize backButton;
@synthesize waitView;


#pragma mark - Lifecycle Functions

- (void)initBuschJaegerSettingsView {
    scanController = [[ZBarReaderViewController alloc] init];
    [scanController setReaderDelegate:self];
}

- (id)init {
    self = [super init];
    if(self != nil) {
        [self initBuschJaegerSettingsView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if(self != nil) {
        [self initBuschJaegerSettingsView];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if(self != nil) {
        [self initBuschJaegerSettingsView];
    }
    return self;
}

- (void)dealloc {
    [scanController release];
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    /* init gradients */
    {
        UIColor* col1 = BUSCHJAEGER_NORMAL_COLOR;
        UIColor* col2 = BUSCHJAEGER_NORMAL_COLOR2;
        
        [BuschJaegerUtils createGradientForView:scanButton withTopColor:col1 bottomColor:col2 cornerRadius:BUSCHJAEGER_DEFAULT_CORNER_RADIUS];
        [BuschJaegerUtils createGradientForView:backButton withTopColor:col1 bottomColor:col2 cornerRadius:BUSCHJAEGER_DEFAULT_CORNER_RADIUS];
    }
    [waitView setHidden:TRUE];
}


#pragma mark - Action Functions

- (IBAction)onScanClick:(id)sender {
    [self presentModalViewController:scanController animated:FALSE];
}

- (IBAction)onBackClick:(id)sender {
    [[BuschJaegerMainView instance].navigationController popViewControllerAnimated:FALSE];
}


#pragma mark - ZBarReaderDelegate Functions

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
    ZBarSymbolSet *zbs = [info objectForKey:ZBarReaderControllerResults];
    if(zbs != nil) {
        BOOL handled = FALSE;
        for(ZBarSymbol *symbol in zbs) {
            if([[[LinphoneManager instance] configuration] parseQRCode:[symbol data] delegate:self]) {
                handled = TRUE;
                [waitView setHidden:FALSE];
            }
        }
        if(handled) {
            [self dismissModalViewControllerAnimated:FALSE];
        }
    }
}


#pragma mark - BuschJaegerConfigurationDelegate Functions

- (void)buschJaegerConfigurationSuccess {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Provisioning Success",nil)
                                                        message:NSLocalizedString(@"The providing configuration is successfully applied", nil)
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    [waitView setHidden:TRUE];
    NSDictionary *dict = [NSDictionary dictionaryWithObject:[[LinphoneManager instance] configuration] forKey:@"configuration"];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneConfigurationUpdate object:self userInfo:dict];
    [[[LinphoneManager instance] configuration] saveFile:kLinphoneConfigurationPath];
}

- (void)buschJaegerConfigurationError:(NSString *)error {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Provisioning error",nil)
                                                        message:[NSString stringWithFormat:NSLocalizedString(@"Connection issue: %@", nil), error]
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    [waitView setHidden:TRUE];
    NSDictionary *dict = [NSDictionary dictionaryWithObject:[[LinphoneManager instance] configuration] forKey:@"configuration"];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneConfigurationUpdate object:self userInfo:dict];
    [[[LinphoneManager instance] configuration] saveFile:kLinphoneConfigurationPath];
}

@end
