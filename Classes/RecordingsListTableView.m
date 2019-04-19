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

#pragma mark - Lifecycle Functions

- (void)initRecordingsTableViewController {
    recordings = [NSMutableDictionary dictionary];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    writablePath = [paths objectAtIndex:0];
    writablePath = [writablePath stringByAppendingString:@"/"];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if (![self selectFirstRow]) {
        //TODO: Make first cell expand to show player
    }
    [self loadData];
}

- (id)init {
    self = [super init];
    if (self) {
        [self initRecordingsTableViewController];
    }
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
    LOGI(@"====>>>> Load recording list - Start");
    
    recordings = [NSMutableDictionary dictionary];
    NSArray *directoryContent = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:writablePath error:NULL];
    for (NSString *file in directoryContent) {
        if (![file hasPrefix:@"recording_"]) {
            continue;
        }
        NSArray *parsedName = [LinphoneUtils parseRecordingName:file];
        NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
        [dateFormat setDateFormat:@"EEEE, MMM d, yyyy"];
        NSString *dayPretty = [dateFormat stringFromDate:[parsedName objectAtIndex:1]];
        NSMutableArray *recOfDay = [recordings objectForKey:dayPretty];
        if (recOfDay) {
            // Loop through the object until a later object, then insert it right before
            int i;
            for (i = 0; i < [recOfDay count]; ++i) {
                NSString *fileAtIndex = [recOfDay objectAtIndex:i];
                NSArray *parsedNameAtIndex = [LinphoneUtils parseRecordingName:fileAtIndex];
                if ([[parsedName objectAtIndex:1] compare:[parsedNameAtIndex objectAtIndex:1]] == NSOrderedDescending) {
                    break;
                }
            }
            [recOfDay insertObject:[writablePath stringByAppendingString:file] atIndex:i];
        } else {
            recOfDay = [NSMutableArray arrayWithObjects:[writablePath stringByAppendingString:file], nil];
            [recordings setObject:recOfDay forKey:dayPretty];
        }
    }
    
    
    LOGI(@"====>>>> Load recording list - End");
    [super loadData];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSIndexPath *selectedRow = [self.tableView indexPathForSelectedRow];
    if (selectedRow && [selectedRow compare:indexPath] == NSOrderedSame) {
        return 150;
    } else {
        return 40;
    }
}

#pragma mark - UITableViewDataSource Functions

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return nil;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return [recordings count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    NSArray *sortedKey = [self getSortedKeys];
    return [(NSArray *)[recordings objectForKey:[sortedKey objectAtIndex:section]] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSString *kCellId = NSStringFromClass(UIRecordingCell.class);
    UIRecordingCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[UIRecordingCell alloc] initWithIdentifier:kCellId];
    }
    NSString *date = [[self getSortedKeys] objectAtIndex:[indexPath section]];
    NSMutableArray *subAr = [recordings objectForKey:date];
    NSString *recordingPath = subAr[indexPath.row];
    [cell setRecording:recordingPath];
    [super accessoryForCell:cell atPath:indexPath];
    //accessoryForCell set it to gray but we don't want it
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    [cell updateFrame];
    return cell;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    CGRect frame = CGRectMake(0, 0, tableView.frame.size.width, tableView.sectionHeaderHeight);
    UIView *tempView = [[UIView alloc] initWithFrame:frame];
    tempView.backgroundColor = [UIColor whiteColor];
    
    UILabel *tempLabel = [[UILabel alloc] initWithFrame:frame];
    tempLabel.backgroundColor = [UIColor clearColor];
    tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]];
    tempLabel.text = [[self getSortedKeys] objectAtIndex:section];
    tempLabel.textAlignment = NSTextAlignmentCenter;
    tempLabel.font = [UIFont boldSystemFontOfSize:17];
    tempLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    [tempView addSubview:tempLabel];
    
    return tempView;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [super tableView:tableView didSelectRowAtIndexPath:indexPath];
    if (![self isEditing]) {
        [tableView beginUpdates];
        [(UIRecordingCell *)[self tableView:tableView cellForRowAtIndexPath:indexPath] updateFrame];
        [tableView endUpdates];
    }
}

- (void)tableView:(UITableView *)tableView
commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        [NSNotificationCenter.defaultCenter removeObserver:self];
        [tableView beginUpdates];
        
        
        NSString *date = [[self getSortedKeys] objectAtIndex:[indexPath section]];
        NSMutableArray *subAr = [recordings objectForKey:date];
        NSString *recordingPath = subAr[indexPath.row];
        [subAr removeObjectAtIndex:indexPath.row];
        if (subAr.count == 0) {
            [recordings removeObjectForKey:date];
            [tableView deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section]
                     withRowAnimation:UITableViewRowAnimationFade];
        }
        
        UIRecordingCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
        [cell setRecording:NULL];
        
        remove([recordingPath cStringUsingEncoding:NSUTF8StringEncoding]);
        
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        [tableView endUpdates];
        
        [self loadData];
    }
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
    [super removeSelectionUsing:^(NSIndexPath *indexPath) {
        [NSNotificationCenter.defaultCenter removeObserver:self];
        
        NSString *date = [[self getSortedKeys] objectAtIndex:[indexPath section]];
        NSMutableArray *subAr = [recordings objectForKey:date];
        NSString *recordingPath = subAr[indexPath.row];
        [subAr removeObjectAtIndex:indexPath.row];
        if (subAr.count == 0) {
            [recordings removeObjectForKey:date];
        }
        UIRecordingCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
        [cell setRecording:NULL];
        remove([recordingPath cStringUsingEncoding:NSUTF8StringEncoding]);
    }];
}

- (void)setSelected:(NSString *)filepath {
    NSArray *parsedName = [LinphoneUtils parseRecordingName:filepath];
    NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
    [dateFormat setDateFormat:@"EEEE, MMM d, yyyy"];
    NSString *dayPretty = [dateFormat stringFromDate:[parsedName objectAtIndex:1]];
    NSUInteger section;
    NSArray *keys = [recordings allKeys];
    for (section = 0; section < [keys count]; ++section) {
        if ([dayPretty isEqualToString:(NSString *)[keys objectAtIndex:section]]) {
            break;
        }
    }
    NSUInteger row;
    NSArray *recs = [recordings objectForKey:dayPretty];
    for (row = 0; row < [recs count]; ++row) {
        if ([filepath isEqualToString:(NSString *)[recs objectAtIndex:row]]) {
            break;
        }
    }
    NSUInteger indexes[] = {section, row};
    [self.tableView selectRowAtIndexPath:[NSIndexPath indexPathWithIndexes:indexes length:2] animated:TRUE scrollPosition:UITableViewScrollPositionNone];
}

#pragma mark - Utilities

- (NSArray *)getSortedKeys {
    return [[recordings allKeys] sortedArrayUsingComparator:^NSComparisonResult(NSString *day2, NSString *day1){
        NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
        [dateFormat setDateFormat:@"EEEE, MMM d, yyyy"];
        NSDate *date1 = [dateFormat dateFromString:day1];
        NSDate *date2 = [dateFormat dateFromString:day2];
        return [date1 compare:date2];
    }];
}


@end
