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

@interface UICallBar: UIViewController {
    UIPauseButton*      pauseButton;
    UIButton*           conferenceButton;
    UIVideoButton*      videoButton;
    UIMicroButton*      microButton;
    UISpeakerButton*    speakerButton;   
    UIButton*           optionsButton;
    UIHangUpButton*     hangupButton;
    UIView*             padView;
    
    //Key pad
	UIDigitButton*  oneButton;
	UIDigitButton*  twoButton;
	UIDigitButton*  threeButton;
	UIDigitButton*  fourButton;
	UIDigitButton*  fiveButton;
	UIDigitButton*  sixButton;
	UIDigitButton*  sevenButton;
	UIDigitButton*  eightButton;
	UIDigitButton*  nineButton;
	UIDigitButton*  starButton;
	UIDigitButton*  zeroButton;
	UIDigitButton*  hashButton;
}

@property (nonatomic, retain) IBOutlet UIPauseButton*   pauseButton;
@property (nonatomic, retain) IBOutlet UIButton*        conferenceButton;
@property (nonatomic, retain) IBOutlet UIVideoButton*   videoButton;
@property (nonatomic, retain) IBOutlet UIMicroButton*   microButton;
@property (nonatomic, retain) IBOutlet UISpeakerButton* speakerButton;
@property (nonatomic, retain) IBOutlet UIButton*        optionsButton;
@property (nonatomic, retain) IBOutlet UIHangUpButton*  hangupButton;
@property (nonatomic, retain) IBOutlet UIView*          padView;

@property (nonatomic, retain) IBOutlet UIButton* oneButton;
@property (nonatomic, retain) IBOutlet UIButton* twoButton;
@property (nonatomic, retain) IBOutlet UIButton* threeButton;
@property (nonatomic, retain) IBOutlet UIButton* fourButton;
@property (nonatomic, retain) IBOutlet UIButton* fiveButton;
@property (nonatomic, retain) IBOutlet UIButton* sixButton;
@property (nonatomic, retain) IBOutlet UIButton* sevenButton;
@property (nonatomic, retain) IBOutlet UIButton* eightButton;
@property (nonatomic, retain) IBOutlet UIButton* nineButton;
@property (nonatomic, retain) IBOutlet UIButton* starButton;
@property (nonatomic, retain) IBOutlet UIButton* zeroButton;
@property (nonatomic, retain) IBOutlet UIButton* hashButton;

- (IBAction)onOptionsClick:(id)sender;
- (IBAction)onConferenceClick:(id)sender;
- (IBAction)onPadClick:(id)sender;

@end
