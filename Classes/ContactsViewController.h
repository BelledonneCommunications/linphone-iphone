/* ContactsViewController.h
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

#import <UIKit/UIKit.h>

#import "UICompositeViewController.h"
#import "ContactsTableViewController.h"

typedef enum _ContactSelectionMode {
    ContactSelectionModeNone,
    ContactSelectionModeEdit,
    ContactSelectionModePhone,
    ContactSelectionModeMessage
} ContactSelectionMode;

@interface ContactSelection : NSObject {
}

+ (void)setSelectionMode:(ContactSelectionMode)selectionMode;
+ (ContactSelectionMode)getSelectionMode;
+ (void)setAddAddress:(NSString*)address;
+ (NSString*)getAddAddress;
/* define sip filter, can be * or sip domain*/
+ (void)setSipFilter:(NSString*) domain;
+ (NSString*)getSipFilter;
+ (void)setEmailFilter:(BOOL)enable;
+ (BOOL)getEmailFilter;

@end

@interface ContactsViewController : UIViewController<UICompositeViewDelegate> {
}

@property (nonatomic, retain) IBOutlet ContactsTableViewController* tableController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UIButton* allButton;
@property (nonatomic, retain) IBOutlet UIButton* linphoneButton;
@property (nonatomic, retain) IBOutlet UIButton *backButton;
@property (nonatomic, retain) IBOutlet UIButton *addButton;

- (IBAction)onAllClick:(id)event;
- (IBAction)onLinphoneClick:(id)event;
- (IBAction)onAddContactClick:(id)event;
- (IBAction)onBackClick:(id)event;

@end
