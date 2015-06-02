//
//  InAppProductsCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 15/04/15.
//
//

#import <UIKit/UIKit.h>
#import <StoreKit/StoreKit.h>

@interface InAppProductsCell : UITableViewCell {
}
@property (strong, nonatomic) IBOutlet UILabel *ptitle;
@property (strong, nonatomic) IBOutlet UILabel *pdescription;
@property (strong, nonatomic) IBOutlet UILabel *pprice;
@property (strong, nonatomic) IBOutlet UISwitch *ppurchased;
@property (nonatomic) BOOL isMaximized;
@property (strong, nonatomic) NSString *productID;

- (id)initWithIdentifier:(NSString*)identifier maximized:(bool)maximized;

+ (CGFloat)getHeight:(BOOL)maximized;

- (void)fillFromProduct:(SKProduct*)prod;

@end
