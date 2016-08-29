//
//  CountryListView.h
//  Country List
//
//  Created by Pradyumna Doddala on 18/12/13.
//  Copyright (c) 2013 Pradyumna Doddala. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PhoneMainView.h"

@protocol CountryListViewDelegate <NSObject,UISearchDisplayDelegate,UISearchBarDelegate>
- (void)didSelectCountry:(NSDictionary *)country;
@end

@interface CountryListView : UIViewController<UICompositeViewDelegate>

@property (nonatomic, weak) id<CountryListViewDelegate>delegate;

- (void)filterContentForSearchText:(NSString*)searchText scope:(NSString*)scope;

- (IBAction)onCancelClick:(id)sender;

+ (NSDictionary *)countryWithIso:(NSString*)iso;

+ (NSDictionary *)countryWithCountryCode:(NSString*)cc;

@end
