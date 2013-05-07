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

@property (nonatomic, retain) IBOutlet UIPauseButton*   pauseButton;
@property (nonatomic, retain) IBOutlet UIButton*        conferenceButton;
@property (nonatomic, retain) IBOutlet UIVideoButton*   videoButton;
@property (nonatomic, retain) IBOutlet UIMicroButton*   microButton;
@property (nonatomic, retain) IBOutlet UISpeakerButton* speakerButton;
@property (nonatomic, retain) IBOutlet UIToggleButton*  routesButton;
@property (nonatomic, retain) IBOutlet UIToggleButton*  optionsButton;
@property (nonatomic, retain) IBOutlet UIHangUpButton*  hangupButton;
@property (nonatomic, retain) IBOutlet UIView*          padView;
@property (nonatomic, retain) IBOutlet UIView*          routesView;
@property (nonatomic, retain) IBOutlet UIView*          optionsView;
@property (nonatomic, retain) IBOutlet UIButton*        routesReceiverButton;
@property (nonatomic, retain) IBOutlet UIButton*        routesSpeakerButton;
@property (nonatomic, retain) IBOutlet UIButton*        routesBluetoothButton;
@property (nonatomic, retain) IBOutlet UIButton*        optionsAddButton;
@property (nonatomic, retain) IBOutlet UIButton*        optionsTransferButton;
@property (nonatomic, retain) IBOutlet UIToggleButton*  dialerButton;

@property (nonatomic, retain) IBOutlet UIDigitButton* oneButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* twoButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* threeButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* fourButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* fiveButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* sixButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* sevenButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* eightButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* nineButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* starButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* zeroButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* sharpButton;

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
