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

- (void)viewDidLoad {
    [super viewDidLoad];
    /* init gradients */
    {
        UIColor* col1 = [UIColor colorWithRed:32.0/255 green:45.0/255 blue:62.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:18.0/255 green:26.0/255 blue:41.0/255 alpha:1.0];
        
        [BuschJaegerUtils createGradientForView:scanButton withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForView:backButton withTopColor:col1 bottomColor:col2];
    }
    [waitView setHidden:TRUE];
}

- (IBAction)onScanClick:(id)sender {
    [self presentModalViewController:scanController animated:TRUE];
}

- (IBAction)onBackClick:(id)sender {
    [[BuschJaegerMainView instance].navigationController popViewControllerAnimated:TRUE];
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
    ZBarSymbolSet *zbs = [info objectForKey:ZBarReaderControllerResults];
    if(zbs != nil) {
        BOOL handled = FALSE;
        for(ZBarSymbol *symbol in zbs) {
            /*if([[[LinphoneManager instance] configuration] parseQRCode:[symbol data] delegate:self]) {
                handled = TRUE;
                [waitView setHidden:FALSE];
            }*/
        }
        if(handled) {
            [self dismissModalViewControllerAnimated:TRUE];
        }
    }
}

- (void)buschJaegerConfigParserSuccess {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Provisioning Success",nil)
                                                        message:NSLocalizedString(@"The providing configuration is successfully applied", nil)
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    [waitView setHidden:TRUE];
}

- (void)buschJaegerConfigParserError:(NSString *)error {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Provisioning error",nil)
                                                        message:[NSString stringWithFormat:NSLocalizedString(@"Connection issue: %@", nil), error]
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    [waitView setHidden:TRUE];
}

@end
