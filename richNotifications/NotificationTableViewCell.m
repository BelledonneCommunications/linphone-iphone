//
//  NotificationTableViewCell.m
//  richNotifications
//
//  Created by David Idmansour on 26/06/2018.
//

#import "NotificationTableViewCell.h"

@implementation NotificationTableViewCell

- (void)awakeFromNib {
    [super awakeFromNib];
    // Initialization code
    _contactImage.layer.cornerRadius = _contactImage.frame.size.height / 2;
    _contactImage.clipsToBounds = YES;
    [self.contentView sendSubviewToBack:_background];
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect bubbleFrame = self.contentView.frame;
    int originX;
        
    bubbleFrame.size = CGSizeMake(_width, _height);

    originX = (_isOutgoing ? self.frame.size.width - bubbleFrame.size.width - 5 : 5);
        
    bubbleFrame.origin.x = originX;
    self.contentView.frame = bubbleFrame;
    
    _msgText.textContainerInset = UIEdgeInsetsZero;
    _msgText.textContainer.lineFragmentPadding = 0;
}

#pragma mark - Bubble size computing

- (CGSize)computeBoundingBox:(NSString *)text size:(CGSize)size font:(UIFont *)font {
    if (!text || text.length == 0)
        return CGSizeMake(0, 0);
    
    return [text boundingRectWithSize:size
                              options:(NSStringDrawingUsesLineFragmentOrigin |
                                       NSStringDrawingTruncatesLastVisibleLine | NSStringDrawingUsesFontLeading)
                           attributes:@{
                                        NSFontAttributeName : font
                                        }
                              context:nil].size;
}

static const CGFloat CELL_MIN_HEIGHT = 60.0f;
static const CGFloat CELL_MIN_WIDTH = 190.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 78 + 10.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 52; // 44;

- (CGSize)ViewHeightForMessage:(NSString *)messageText withWidth:(int)width {
    static UIFont *messageFont = nil;
    
    if (!messageFont) {
        messageFont = _msgText.font;
    }
    CGSize size;
    size = [self computeBoundingBox:messageText
                               size:CGSizeMake(width - CELL_MESSAGE_X_MARGIN - 4, CGFLOAT_MAX)
                               font:messageFont];
    
    size.width = MAX(size.width + CELL_MESSAGE_X_MARGIN, CELL_MIN_WIDTH);
    size.height = MAX(size.height + CELL_MESSAGE_Y_MARGIN, CELL_MIN_HEIGHT);
    return size;
}
- (CGSize)ViewSizeForMessage:(NSString *)chat withWidth:(int)width {
    static UIFont *dateFont = nil;
    static CGSize dateViewSize;
    
    if (!dateFont) {
        dateFont = _nameDate.font;
        dateViewSize = _nameDate.frame.size;
        dateViewSize.width = CGFLOAT_MAX;
    }
    
    CGSize messageSize = [self ViewHeightForMessage:chat withWidth:width];
    CGSize dateSize = [self computeBoundingBox:_nameDate.text size:dateViewSize font:dateFont];
    messageSize.width = MAX(MAX(messageSize.width, MIN(dateSize.width + CELL_MESSAGE_X_MARGIN, width)), CELL_MIN_WIDTH);
    
    return messageSize;
}

@end
