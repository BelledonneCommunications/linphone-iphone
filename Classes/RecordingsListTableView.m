//
//  RecordingsListTableView.m
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

#import "RecordingsListTableView.h"
#import "UIRecordingCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils.h"

@implementation RecordingsListTableView
NSArray *sortedRecordings;

#pragma mark - Lifecycle Functions

- (void)initRecordingsTableViewController {
    recordings = [[OrderedDictionary alloc] init];
    sortedRecordings = [[NSArray alloc] init];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if (![self selectFirstRow]) {
        //TODO: Make first cell expand to show player
    }
}

- (id)init {
    self = [super init];
    if (self) {
        [self initRecordingsTableViewController];
    }
    _ongoing = FALSE;
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
        [self initRecordingsTableViewController];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self removeAllRecordings];
}

- (void)removeAllRecordings {
    for (NSInteger j = 0; j < [self.tableView numberOfSections]; ++j) {
        for (NSInteger i = 0; i < [self.tableView numberOfRowsInSection:j]; ++i) {
            [[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:j]] setRecording:nil];
        }
    }
}


- (void)loadData {
    _ongoing = TRUE;
    LOGI(@"====>>>> Load recording list - Start");
    
    //Clear recording cells
    for (NSInteger j = 0; j < [self.tableView numberOfSections]; ++j){
        for (NSInteger i = 0; i < [self.tableView numberOfRowsInSection:j]; ++i)
        {
            [[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:j]] setRecording:nil];
        }
    }
    
    //TODO: Fill the recordings dictionnary with the recording names, keys are dates
    
    LOGI(@"====>>>> Load recording list - End");
    [super loadData];
    _ongoing = FALSE;
}

#pragma mark - UITableViewDataSource Functions

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return [recordings allKeys];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [recordings count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [(OrderedDictionary *)[recordings objectForKey:[recordings keyAtIndex:section]] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSString *kCellId = NSStringFromClass(UIRecordingCell.class);
    UIRecordingCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[UIRecordingCell alloc] initWithIdentifier:kCellId];
    }
    NSString *recording = @"";
    //TODO: Set recording to the path of the recording
    [cell setRecording:recording];
    [super accessoryForCell:cell atPath:indexPath];
    
    return cell;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    CGRect frame = CGRectMake(0, 0, tableView.frame.size.width, tableView.sectionHeaderHeight);
    UIView *tempView = [[UIView alloc] initWithFrame:frame];
    tempView.backgroundColor = [UIColor whiteColor];
    
    UILabel *tempLabel = [[UILabel alloc] initWithFrame:frame];
    tempLabel.backgroundColor = [UIColor clearColor];
    tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]];
    tempLabel.text = [recordings keyAtIndex:section];
    tempLabel.textAlignment = NSTextAlignmentCenter;
    tempLabel.font = [UIFont boldSystemFontOfSize:17];
    tempLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    [tempView addSubview:tempLabel];
    
    return tempView;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [super tableView:tableView didSelectRowAtIndexPath:indexPath];
    if (![self isEditing]) {
        //TODO: Expand selected cell to display player
    }
}

- (void)tableView:(UITableView *)tableView
commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        [NSNotificationCenter.defaultCenter removeObserver:self];
        [tableView beginUpdates];
        
        
        NSString *date = [recordings keyAtIndex:[indexPath section]];
        NSMutableArray *subAr = [recordings objectForKey:date];
        //NSString *recording = subAr[indexPath.row];
        [subAr removeObjectAtIndex:indexPath.row];
        if (subAr.count == 0) {
            [recordings removeObjectForKey:date];
            [tableView deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section]
                     withRowAnimation:UITableViewRowAnimationFade];
        }
        
        UIRecordingCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
        [cell setRecording:NULL];
        //TODO: Delete recording file here
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        [tableView endUpdates];
        
        [self loadData];
    }
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
    [super removeSelectionUsing:^(NSIndexPath *indexPath) {
        [NSNotificationCenter.defaultCenter removeObserver:self];
        
        NSString *date = [recordings keyAtIndex:[indexPath section]];
        NSMutableArray *subAr = [recordings objectForKey:date];
        //NSString *recording = subAr[indexPath.row];
        [subAr removeObjectAtIndex:indexPath.row];
        if (subAr.count == 0) {
            [recordings removeObjectForKey:date];
        }
        UIRecordingCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
        [cell setRecording:NULL];
        //TODO: Delete recording file here
    }];
}


@end
