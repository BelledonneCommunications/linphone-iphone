//
//  UIChatConversationInfoTableViewCell.m
//  linphone
//
//  Created by REIS Benjamin on 23/10/2017.
//

#import "PhoneMainView.h"
#import "UIChatConversationInfoTableViewCell.h"

@implementation UIChatConversationInfoTableViewCell

- (void)awakeFromNib {
    [super awakeFromNib];
    // Initialization code
}

- (id)initWithIdentifier:(NSString *)identifier {
	self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier];
	if (self != nil) {
		NSArray *arrayOfViews =
		[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		if ([arrayOfViews count] >= 1) {
			UIChatConversationInfoTableViewCell *sub = ((UIChatConversationInfoTableViewCell *)[arrayOfViews objectAtIndex:0]);
			self = sub;
		}
	}

	UITapGestureRecognizer *adminTap = [[UITapGestureRecognizer alloc]
										initWithTarget:self
										action:@selector(onAdmin)];
	adminTap.delegate = self;
	adminTap.numberOfTapsRequired = 1;
	[_adminButton addGestureRecognizer:adminTap];
	return self;
}

- (IBAction)onDelete:(id)sender {
	[_controllerView.contacts removeObject:_uri];
	if ([_controllerView.admins containsObject:_uri])
	   [_controllerView.admins removeObject:_uri];

	[_controllerView.tableView reloadData];
	_controllerView.nextButton.enabled = _controllerView.nameLabel.text.length > 0 && _controllerView.contacts.count > 0;
}

- (void)onAdmin {
	_adminLabel.enabled = !_adminLabel.enabled;
	NSString *content = _adminLabel.enabled
						? @"check_selected.png"
						: @"check_unselected.png";
	
	_adminImage.image = [UIImage imageNamed:content];

	if (_adminLabel.enabled)
		[_controllerView.admins addObject:_uri];
	else
		[_controllerView.admins removeObject:_uri];
}

@end
