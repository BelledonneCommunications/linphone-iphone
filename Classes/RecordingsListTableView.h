//
//  RecordingsListTableView.h
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import <UIKit/UIKit.h>

#import "UICheckBoxTableView.h"

@interface RecordingsListTableView : UICheckBoxTableView {
@private
    NSMutableDictionary *recordings;
    //This has sub arrays indexed with the date of the recordings, themselves containings the recordings.
    NSString *writablePath;
    //This is the path to the folder where we write the recordings to. We should probably define it in LinphoneManager though.
}
- (void)loadData;
- (void)removeAllRecordings;
- (void)setSelected:(NSString *)filepath;

@end
