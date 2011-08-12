/* LinphoneUIControler.h
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
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */     
#import <UIKit/UIKit.h>


@protocol LinphoneUICallDelegate
// UI changes
-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName;
-(void) displayCallInProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName;
-(void) displayIncomingCallNotigicationFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName;
-(void) displayIncallFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName;
//status reporting
-(void) displayStatus:(NSString*) message; 
@end


@protocol LinphoneUIRegistrationDelegate
// UI changes for registration
-(void) displayRegisteredFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain ;
-(void) displayRegisteringFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain ;
-(void) displayRegistrationFailedFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain forReason:(NSString*) reason;
-(void) displayNotRegisteredFromUI:(UIViewController*) viewCtrl; 
@end

