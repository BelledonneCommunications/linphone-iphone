/* UICallBar.h
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

#import <UIKit/UIKit.h>

#import "UIMicroButton.h"
#import "UIPauseButton.h"
#import "UISpeakerButton.h"
#import "UIVideoButton.h"
#import "UIHangUpButton.h"
#import "UIDigitButton.h"
#import "TPMultiLayoutViewController.h"

@interface UICallBar: TPMultiLayoutViewController {
}

@property (nonatomic, strong) IBOutlet UIPauseButton*   pauseButton;
@property (nonatomic, strong) IBOutlet UIButton*        conferenceButton;
@property (nonatomic, strong) IBOutlet UIVideoButton*   videoButton;
@property (nonatomic, strong) IBOutlet UIMicroButton*   microButton;
@property (nonatomic, strong) IBOutlet UISpeakerButton* speakerButton;
@property (nonatomic, strong) IBOutlet UIToggleButton*  routesButton;
@property (nonatomic, strong) IBOutlet UIToggleButton*  optionsButton;
@property (nonatomic, strong) IBOutlet UIHangUpButton*  hangupButton;
@property (nonatomic, strong) IBOutlet UIView*          padView;
@property (nonatomic, strong) IBOutlet UIView*          routesView;
@property (nonatomic, strong) IBOutlet UIView*          optionsView;
@property (nonatomic, strong) IBOutlet UIButton*        routesReceiverButton;
@property (nonatomic, strong) IBOutlet UIButton*        routesSpeakerButton;
@property (nonatomic, strong) IBOutlet UIButton*        routesBluetoothButton;
@property (nonatomic, strong) IBOutlet UIButton*        optionsAddButton;
@property (nonatomic, strong) IBOutlet UIButton*        optionsTransferButton;
@property (nonatomic, strong) IBOutlet UIToggleButton*  dialerButton;

@property (nonatomic, strong) IBOutlet UIImageView*         leftPadding;
@property (nonatomic, strong) IBOutlet UIImageView*         rightPadding;


@property (nonatomic, strong) IBOutlet UIDigitButton* oneButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* twoButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* threeButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* fourButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* fiveButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* sixButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* sevenButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* eightButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* nineButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* starButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* zeroButton;
@property (nonatomic, strong) IBOutlet UIDigitButton* sharpButton;

- (IBAction)onRoutesClick:(id)sender;
- (IBAction)onRoutesBluetoothClick:(id)sender;
- (IBAction)onRoutesReceiverClick:(id)sender;
- (IBAction)onRoutesSpeakerClick:(id)sender;
- (IBAction)onOptionsClick:(id)sender;
- (IBAction)onOptionsTransferClick:(id)sender;
- (IBAction)onOptionsAddClick:(id)sender;
- (IBAction)onConferenceClick:(id)sender;
- (IBAction)onPadClick:(id)sender;

@end
