//
//  UIChatNotifiedEventCell.m
//  linphone
//
//  Created by REIS Benjamin on 30/10/2017.
//

#import <Foundation/Foundation.h>
#import "UIChatNotifiedEventCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>

@implementation UIChatNotifiedEventCell

#pragma mark - Class methods
static const CGFloat NOTIFIED_CELL_HEIGHT = 44;

+ (CGFloat)height {
	return NOTIFIED_CELL_HEIGHT;
}

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		if ([identifier isEqualToString:NSStringFromClass(self.class)]) {
			NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
			// resize cell to match .nib size. It is needed when resized the cell to
			// correctly adapt its height too
			UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:arrayOfViews.count - 1]);
			[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
			[self addSubview:sub];
		}
	}
	_event = NULL;
	return self;
}

- (void)dealloc {
	_event = NULL;
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	_leftBar.hidden = _rightBar.hidden = editing;
	if (editing)
		[_contactDateLabel setFrame:CGRectMake(1, 1, _contactDateLabel.frame.size.width, NOTIFIED_CELL_HEIGHT)];
}

#pragma mark -

- (void)setEvent:(LinphoneEventLog *)event {
	_event = event;
	NSString *eventString;
	switch (linphone_event_log_get_type(event)) {
		case LinphoneEventLogTypeConferenceSubjectChanged: {
			NSString *subject = [NSString stringWithUTF8String:linphone_event_log_get_subject(event) ?: LINPHONE_DUMMY_SUBJECT];
			NSString *formatedString = [NSString stringWithFormat:@"Chat room subject has changed to : %@", subject];
			eventString = NSLocalizedString(formatedString, nil);
			break;
		}
		case LinphoneEventLogTypeConferenceParticipantAdded: {
			NSString *participant = [FastAddressBook displayNameForAddress:linphone_event_log_get_participant_address(event)];
			NSString *formatedString = [NSString stringWithFormat:@"%@ has been added to the chat room", participant];
			eventString = NSLocalizedString(formatedString, nil);
			break;
		}
		case LinphoneEventLogTypeConferenceParticipantRemoved: {
			NSString *participant = [FastAddressBook displayNameForAddress:linphone_event_log_get_participant_address(event)];
			NSString *formatedString = [NSString stringWithFormat:@"%@ has been removed of the chat room", participant];
			eventString = NSLocalizedString(formatedString, nil);
			break;
		}
		case LinphoneEventLogTypeConferenceParticipantSetAdmin: {
			NSString *participant = [FastAddressBook displayNameForAddress:linphone_event_log_get_participant_address(event)];
			NSString *formatedString = [NSString stringWithFormat:@"%@ is now an admin of the chat room", participant];
			eventString = NSLocalizedString(formatedString, nil);
			break;
		}
		case LinphoneEventLogTypeConferenceParticipantUnsetAdmin: {
			NSString *participant = [FastAddressBook displayNameForAddress:linphone_event_log_get_participant_address(event)];
			NSString *formatedString = [NSString stringWithFormat:@"%@ is no longer an admin of the chat room", participant];
			eventString = NSLocalizedString(formatedString, nil);
			break;
		}
		default:
			return;
	}
	_contactDateLabel.text = eventString;

	CGSize newSize = [_contactDateLabel.text boundingRectWithSize:CGSizeZero
														  options:(NSStringDrawingUsesLineFragmentOrigin |
																   NSStringDrawingTruncatesLastVisibleLine | NSStringDrawingUsesFontLeading)
													   attributes:@{NSFontAttributeName :_contactDateLabel.font}
														  context:nil].size;
	float delta = (_contactDateLabel.frame.size.width - newSize.width) / 2;

	[_contactDateLabel setFrame:CGRectMake((_eventView.frame.size.width - newSize.width) / 2, 1, newSize.width, NOTIFIED_CELL_HEIGHT)];
	[_leftBar setFrame:CGRectMake(0,
								  _leftBar.frame.origin.y,
								  _contactDateLabel.frame.origin.x - 5,
								  1)];
	[_rightBar setFrame:CGRectMake(_contactDateLabel.frame.origin.x + newSize.width + 5,
								   _rightBar.frame.origin.y,
								   _rightBar.frame.size.width + delta,
								   1)];
}

- (void)layoutSubviews {
	[super layoutSubviews];
}

@end
