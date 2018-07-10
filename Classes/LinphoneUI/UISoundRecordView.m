//
//  UISoundRecordView.m
//  linphone
//
//  Created by David Idmansour on 09/07/2018.
//

#import "UISoundRecordView.h"
#import "PhoneMainView.h"

@implementation UISoundRecordView {
    @private
    LinphoneRecordState state;
}

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
        state = LinphoneRecordPaused;
    }
    return self;
}

- (IBAction)onRecord:(UIButton *)sender {
    switch(state) {
        case LinphoneRecording:
            LOGI(@"Record paused");
            state = LinphoneRecordPaused;
            break;
        case LinphoneRecordPaused:
            LOGI(@"Recording...");
            state = LinphoneRecording;
            break;
        case LinphoneRecordFinished:
            LOGI(@"Restarted recording, erasing previous record...");
            state = LinphoneRecording;
            break;
    }
}

- (IBAction)onStop:(UIButton *)sender {
    LOGI(@"Stopped recording");
    state = LinphoneRecordFinished;
}

- (IBAction)onCancel:(UIButton *)sender {
    [VIEW(ChatConversationView) changeToMessageView];
    LOGI(@"Canceled recording");
}
@end
