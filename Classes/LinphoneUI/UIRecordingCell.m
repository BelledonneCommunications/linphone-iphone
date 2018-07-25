//
//  UIRecordingCell.m
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import "UIRecordingCell.h"
#import "PhoneMainView.h"
#import "UILabel+Boldify.h"
#import "Utils.h"

@implementation UIRecordingCell

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        
        // resize cell to match .nib size. It is needed when resized the cell to
        // correctly adapt its height too
        UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:0]);
        [self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
        [self addSubview:sub];
        self.recording = NULL;
    }
    return self;
}

- (void)dealloc {
    self.recording = NULL;
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Property Functions

- (void)setRecording:(NSString *)arecording {
    _recording = arecording;
    if(_recording) {
        //TODO: Parse file name to get name of contact and date
    }
}

#pragma mark -

- (void)touchUp:(id)sender {
    [self setHighlighted:true animated:true];
}

- (void)touchDown:(id)sender {
    [self setHighlighted:false animated:true];
}

- (NSString *)accessibilityLabel {
    return _nameLabel.text;
}

- (void)setEditing:(BOOL)editing {
    [self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    if (animated) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:0.3];
    }
    if (animated) {
        [UIView commitAnimations];
    }
}


@end
