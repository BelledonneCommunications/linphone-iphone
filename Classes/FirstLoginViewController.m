/*
 *  FirstLoginViewController.m
 *
 * Description: 
 *
 *
 * Belledonne Communications (C) 2009
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */



#import "FirstLoginViewController.h"


@implementation FirstLoginViewController

@synthesize ok;
@synthesize identity;
@synthesize passwd;
@synthesize domain;
@synthesize mainDelegate;

@synthesize axtelPin;
@synthesize activityIndicator;
@synthesize site;
/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization
    }
    return self;
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	[identity setText:[[NSUserDefaults standardUserDefaults] stringForKey:@"username_preference"]];
	[passwd setText:[[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"]];
	[domain setText:[[NSUserDefaults standardUserDefaults] stringForKey:@"domain_preference"]];
	[axtelPin setText:[[NSUserDefaults standardUserDefaults] stringForKey:@"axtelpin_preference"]];
}


/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}



-(void) doOk:(id)sender {
	if (sender == site) {
		NSURL *url = [NSURL URLWithString:@"http://dialer.axtellabs.net"];
		[[UIApplication sharedApplication] openURL:url];
		return;
	}
	UIAlertView* error=nil;
		// mandotory parameters
	if ([identity.text length]==0 ) {
		error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
														message:NSLocalizedString(@"Enter your username",nil) 
														delegate:nil 
														cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
														otherButtonTitles:nil];
		goto end;
	} else  {
		[[NSUserDefaults standardUserDefaults] setObject:identity.text forKey:@"username_preference"];
		
		
	}
	if ([passwd.text length]==0 ) {
		 error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
														message:NSLocalizedString(@"Enter your password",nil) 
														delegate:nil 
														cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
														otherButtonTitles:nil];
		goto end;
	} else {
		[[NSUserDefaults standardUserDefaults] setObject:passwd.text forKey:@"password_preference"];
	}
	if ([domain.text length]==0) {
		 error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
														message:NSLocalizedString(@"Enter your domain",nil) 
														delegate:nil 
														cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
														otherButtonTitles:nil];
		goto end;
	} else {
		[[NSUserDefaults standardUserDefaults] setObject:domain.text forKey:@"domain_preference"];
	}
	
	//optionnal parameters
	if ([axtelPin.text length] >0) {
		[[NSUserDefaults standardUserDefaults] setObject:axtelPin.text forKey:@"axtelpin_preference"];
	}
	
	if ([axtelPin.text length] >0) {
		[[NSUserDefaults standardUserDefaults] setBool:true forKey:@"tunnelenable_preference"];
	}

end:
	if (error != nil) {
		[error show];
	} else {
		//[self.view removeFromSuperview:self];
		if ([mainDelegate initProxySettings]) {
			[self.activityIndicator setHidden:false];
		} else {
			error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Error",nil)
												message:NSLocalizedString(@"Wrong domain or network unreachable",nil) 
												delegate:nil 
												cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
												otherButtonTitles:nil];
			[error show];
		}
	};
	
	
}
-(void) authInfoRequested {
	[self.activityIndicator setHidden:true];
}
-(void) callStateChange:(LinphoneGeneralState*) state {
	if (self.view.hidden) {
		//nothing to do, viev is no longer active
		return;
	}
	//	/* states for GSTATE_GROUP_POWER */
	//	GSTATE_POWER_OFF = 0,        /* initial state */
	//	GSTATE_POWER_STARTUP,
	//	GSTATE_POWER_ON,
	//	GSTATE_POWER_SHUTDOWN,
	//	/* states for GSTATE_GROUP_REG */
	//	GSTATE_REG_NONE = 10,       /* initial state */
	//	GSTATE_REG_OK,
	//	GSTATE_REG_FAILED,
	//	/* states for GSTATE_GROUP_CALL */
	//	GSTATE_CALL_IDLE = 20,      /* initial state */
	//	GSTATE_CALL_OUT_INVITE,
	//	GSTATE_CALL_OUT_CONNECTED,
	//	GSTATE_CALL_IN_INVITE,
	//	GSTATE_CALL_IN_CONNECTED,
	//	GSTATE_CALL_END,
	//	GSTATE_CALL_ERROR,
	//	GSTATE_INVALID
	switch (state->new_state) {
		case GSTATE_REG_FAILED: {
			UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Error",nil)
														   message:NSLocalizedString(@"Wrong domain or username",nil) 
														   delegate:nil 
														   cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
														   otherButtonTitles:nil];
			[error show];
			[self.activityIndicator setHidden:true];
			break;
		}
		case GSTATE_REG_OK: {
			[[NSUserDefaults standardUserDefaults] setBool:true forKey:@"firstlogindone_preference"]; 
			//[self.view removeFromSuperview];
			[self.view setHidden:true];
			[self.activityIndicator setHidden:true];
			break;
		}
		default:
			break;
	}
	
}


- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.
    [theTextField resignFirstResponder];
    return YES;
}


@end
