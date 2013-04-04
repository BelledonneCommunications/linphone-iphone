/* WizardViewController.h
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
#import <XMLRPCConnectionDelegate.h>
#import "UICompositeViewController.h"

@interface WizardViewController : TPMultiLayoutViewController <UITextFieldDelegate, UICompositeViewDelegate, XMLRPCConnectionDelegate, UIGestureRecognizerDelegate> {
    @private
    UITextField *activeTextField;
    UIView *currentView;
    NSMutableArray *historyViews;
}

@property (nonatomic, retain) IBOutlet UIScrollView *contentView;

@property (nonatomic, retain) IBOutlet UIView *welcomeView;
@property (nonatomic, retain) IBOutlet UIView *choiceView;
@property (nonatomic, retain) IBOutlet UIView *createAccountView;
@property (nonatomic, retain) IBOutlet UIView *connectAccountView;
@property (nonatomic, retain) IBOutlet UIView *externalAccountView;
@property (nonatomic, retain) IBOutlet UIView *validateAccountView;

@property (nonatomic, retain) IBOutlet UIView *waitView;

@property (nonatomic, retain) IBOutlet UIButton *backButton;
@property (nonatomic, retain) IBOutlet UIButton *startButton;
@property (nonatomic, retain) IBOutlet UIButton *createAccountButton;
@property (nonatomic, retain) IBOutlet UIButton *connectAccountButton;
@property (nonatomic, retain) IBOutlet UIButton *externalAccountButton;

@property (nonatomic, retain) IBOutlet UIImageView *choiceViewLogoImageView;

@property (nonatomic, retain) IBOutlet UITapGestureRecognizer *viewTapGestureRecognizer;

- (void)reset;

- (IBAction)onStartClick:(id)sender;
- (IBAction)onBackClick:(id)sender;
- (IBAction)onCancelClick:(id)sender;

- (IBAction)onCreateAccountClick:(id)sender;
- (IBAction)onConnectAccountClick:(id)sender;
- (IBAction)onExternalAccountClick:(id)sender;
- (IBAction)onCheckValidationClick:(id)sender;

- (IBAction)onSignInClick:(id)sender;
- (IBAction)onSignInExternalClick:(id)sender;
- (IBAction)onRegisterClick:(id)sender;

@end
