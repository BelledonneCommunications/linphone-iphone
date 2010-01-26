/*
 *  ConsoleViewController.h
 *
 * Description: 
 *
 *
 * Belledonne Communications (C) 2010
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */


#import <UIKit/UIKit.h>

@protocol LogView
+(void) addLog:(NSString*) log;

@end

@interface ConsoleViewController : UIViewController <LogView> {
	UITextView* logs;
	UIView*		logsView;
	
	
}
-(void) doAction;
@property (nonatomic, retain) IBOutlet UITextView* logs;
@property (nonatomic, retain) IBOutlet UIButton* clear;
@property (nonatomic, retain) IBOutlet UIView*	logsView;


@end
