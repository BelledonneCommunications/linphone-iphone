//
//  ChatConversationImdnTableViewHeader.m
//  linphone
//
//  Created by REIS Benjamin on 26/04/2018.
//

#import <Foundation/Foundation.h>

#import "ChatConversationImdnTableViewHeader.h"

@implementation ChatConversationImdnTableViewHeader

- (id)initWithIdentifier:(NSString *)identifier {
	self = [super initWithReuseIdentifier:identifier];
	if (self != nil) {
		NSArray *arrayOfViews =
		[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		if ([arrayOfViews count] >= 1) {
			ChatConversationImdnTableViewHeader *sub = ((ChatConversationImdnTableViewHeader *)[arrayOfViews objectAtIndex:0]);
			self = sub;
		}
	}
	return self;
}

@end
