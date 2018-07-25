//
//  RecordingsListView.h
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "RecordingsListTableView.h"
#import "UIIconButton.h"

typedef enum _RecordingSelectionMode { RecordingSelectionModeNone, RecordingSelectionModeEdit } RecordingSelectionMode;

@interface RecordingSelection : NSObject <UISearchBarDelegate> {
}

+ (void)setSelectionMode:(RecordingSelectionMode)selectionMode;
+ (RecordingSelectionMode)getSelectionMode;

@end

@interface RecordingsListView : UIViewController <UICompositeViewDelegate>

@property(strong, nonatomic) IBOutlet RecordingsListTableView *tableController;
@property(strong, nonatomic) IBOutlet UIView *topBar;
@property(weak, nonatomic) IBOutlet UIIconButton *deleteButton;

- (IBAction)onDeleteClick:(id)sender;
- (IBAction)onEditionChangeClick:(id)sender;

@end
