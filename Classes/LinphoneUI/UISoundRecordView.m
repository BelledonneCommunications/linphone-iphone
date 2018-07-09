//
//  UISoundRecordView.m
//  linphone
//
//  Created by David Idmansour on 09/07/2018.
//

#import "UISoundRecordView.h"
#import "PhoneMainView.h"

@implementation UISoundRecordView

- (id)init {
    if (self = [super init]) {
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        UIView *sub = nil;
        for (int i = 0; i < arrayOfViews.count; i++) {
            if ([arrayOfViews[i] isKindOfClass:UIView.class]) {
                sub = arrayOfViews[i];
                break;
            }
        }
        [self addSubview:sub];
        self.recordView = sub;
    }
    return self;
}

- (IBAction)onRecord:(UIButton *)sender {
    NSLog(@"Recording...");
}

- (IBAction)onStop:(UIButton *)sender {
    NSLog(@"Stopped recording");
}

- (IBAction)onCancel:(UIButton *)sender {
    [VIEW(ChatConversationView) changeToMessageView];
    NSLog(@"Canceled recording");
}
@end
