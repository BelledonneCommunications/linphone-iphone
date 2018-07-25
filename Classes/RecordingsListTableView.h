//
//  RecordingsListTableView.h
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import <UIKit/UIKit.h>

#import "UICheckBoxTableView.h"

#import "OrderedDictionary.h"

@interface RecordingsListTableView : UICheckBoxTableView {
@private
    OrderedDictionary *recordings;
    //This has sub arrays indexed with the date of the recordings, themselves containings the recordings.
}
@property(nonatomic) BOOL ongoing;
- (void)loadData;
- (void)removeAllRecordings;

@end
