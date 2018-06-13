//
//  TodayViewController.m
//  latestCallsWidget
//
//  Created by David Idmansour on 06/06/2018.
//

#import "TodayViewController.h"
#import <NotificationCenter/NotificationCenter.h>
#import "linphone/linphonecore.h"
#ifdef __IPHONE_9_0
#import <Contacts/Contacts.h>
#endif

@interface TodayViewController () <NCWidgetProviding>

@end

@implementation TodayViewController

- (instancetype) init {
    printf("BONJOUR\n");
    return [super init];
}

- (void)loadData {
    _contactsToDisplay = [NSMutableArray array];
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    
    NSUserDefaults *mySharedDefaults = [[NSUserDefaults alloc] initWithSuiteName: @"group.belledonne-communications.linphone.widget"];
    NSMutableArray *dates = [NSMutableArray array];
    NSDictionary *logsTmp = [mySharedDefaults objectForKey:@"logs"];
    _logs = [NSMutableDictionary dictionary];
    for (NSString *dateStr in logsTmp.allKeys) {
        NSDictionary *log = [logsTmp objectForKey:dateStr];
        NSDate *date = [formatter dateFromString:dateStr];
        [dates addObject:date];
        [_logs setObject:log forKey:date];
    }
    _sortedDates = [[NSMutableArray alloc]
                    initWithArray:[dates sortedArrayUsingComparator:^NSComparisonResult(NSDate *d1, NSDate *d2) {
        return [d2 compare:d1]; // reverse order
    }]];
}

- (void) fetchContactsInBackGroundThread{
    _imgs = [NSMutableDictionary dictionary];
    CNContactStore *store = [[CNContactStore alloc] init];
    CNEntityType entityType = CNEntityTypeContacts;
    [store requestAccessForEntityType:entityType completionHandler:^(BOOL granted, NSError *_Nullable error) {
        BOOL success = FALSE;
        if(granted){
            //LOGD(@"CNContactStore authorization granted");
            
            NSError *contactError;
            CNContactStore* store = [[CNContactStore alloc] init];
            [store containersMatchingPredicate:[CNContainer predicateForContainersWithIdentifiers:@[ store.defaultContainerIdentifier]] error:&contactError];
            NSArray *keysToFetch = @[
                                     CNContactEmailAddressesKey, CNContactPhoneNumbersKey, CNContactImageDataAvailableKey,
                                     CNContactFamilyNameKey, CNContactGivenNameKey, CNContactNicknameKey,
                                     CNContactInstantMessageAddressesKey, CNContactIdentifierKey, CNContactImageDataKey
                                     ];
            CNContactFetchRequest *request = [[CNContactFetchRequest alloc] initWithKeysToFetch:keysToFetch];
            
            success = [store enumerateContactsWithFetchRequest:request error:&contactError usingBlock:^(CNContact *__nonnull contact, BOOL *__nonnull stop) {
                if (contactError) {
                    NSLog(@"error fetching contacts %@",
                          contactError);
                } else {
                        if (contact.imageDataAvailable) {
                            NSArray *addresses = contact.instantMessageAddresses;
                            [self.imgs setObject:contact.imageData forKey:contact.givenName];
                            printf("Ajout de l'image de %s\n", contact.givenName.UTF8String);
                        }
                }
            }];
            dispatch_semaphore_signal(self.sem);
        }
        
    }];
}

- (void)draw {
    _contactsToDisplay = [NSMutableArray array];
    _logIds = [NSMutableArray array];
    int i = 0, j = 0;
    dispatch_semaphore_wait(_sem, DISPATCH_TIME_FOREVER);
    _nbImgs = [NSNumber numberWithInteger:_imgs.count];
    while (i < _stackViews.count && j < _sortedDates.count) {
        NSDate *date = _sortedDates[j++];
        NSString *address = [[_logs objectForKey:date] objectForKey:@"address"];
        LinphoneAddress *adr = linphone_address_new([address UTF8String]);
        NSString *logId = [[_logs objectForKey:date] objectForKey:@"id"];
        if ([_contactsToDisplay containsObject:[NSString stringWithUTF8String:linphone_address_as_string_uri_only(adr)]])
            continue;
        [_contactsToDisplay addObject:[NSString stringWithUTF8String:linphone_address_as_string_uri_only(adr)]];
        [_logIds addObject:logId];
        NSString *displayName = [NSString stringWithUTF8String:linphone_address_get_display_name(adr)];
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[1];
        // making rounded image
        UIImageView *imageView = button.imageView;
        imageView.layer.cornerRadius = imageView.frame.size.height / 2;
        imageView.clipsToBounds = YES;
        UILabel *name = stack.subviews[2];
        if ([self.imgs.allKeys containsObject:displayName]) {
            [button setImage:[UIImage imageWithData:[_imgs objectForKey:displayName]] forState:UIControlStateNormal];
            printf("Affichage de l'image de %s\n", displayName.UTF8String);
        }
        printf("Fin de l'affichage pour %s\n", displayName.UTF8String);
        [stack setAlpha:1];
        button.enabled = YES;
        [name setText:displayName];
        i++;
    }
    while (i < _stackViews.count) {
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[1];
        [stack setAlpha:0];
        button.enabled = NO;
        i++;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    NSLog(@"------ LA VUE A CHARGE ------");
    _sem = dispatch_semaphore_create(0);
    _nbImgs = [NSNumber numberWithInt:0];
    [self fetchContactsInBackGroundThread];
    [self loadData];
    [self draw];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)widgetPerformUpdateWithCompletionHandler:(void (^)(NCUpdateResult))completionHandler {
    // Perform any setup necessary in order to update the view.
    
    // If an error is encountered, use NCUpdateResultFailed
    // If there's no update required, use NCUpdateResultNoData
    // If there's an update, use NCUpdateResultNewData
    NSLog(@"valeurs dans dict contacts : %i", (int)_imgs.count);
    NSLog(@"valeurs de nbImgs : %i", (int)_nbImgs.integerValue);
    /*if (_nbImgs.integerValue != _imgs.count) {
        [self draw];
        completionHandler(NCUpdateResultNoData);
    } else {
        [self draw];
        completionHandler(NCUpdateResultNoData);
    }*/
    completionHandler(NCUpdateResultNewData);
}

- (void)launchAppWithURL:(NSURL *) url {
    [self.extensionContext openURL:url
                 completionHandler:nil];
}

- (void)launchOnHistoryDetailsWithId:(NSString *)logId {
    [self launchAppWithURL:[NSURL URLWithString:[@"linphone-widget://call_log/show?" stringByAppendingString:logId]]];
}

- (IBAction)firstButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[0]];
}

- (IBAction)secondButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[1]];
}

- (IBAction)thirdButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[2]];
}

- (IBAction)fourthButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[3]];
}
@end
