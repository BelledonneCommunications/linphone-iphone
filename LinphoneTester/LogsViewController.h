//
//  LogsViewController.h
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 01/06/2014.
//
//

#import <UIKit/UIKit.h>


@interface LogsViewController : UIViewController
@property (weak, nonatomic) IBOutlet UITextView *tview;

- (IBAction)clearLogs:(id)sender;

@end
