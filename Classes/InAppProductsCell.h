//
//  InAppProductsCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 15/04/15.
//
//

#import <UIKit/UIKit.h>


@interface InAppProductsCell : UITableViewCell {
}
@property (retain, nonatomic) IBOutlet UILabel *ptitle;
@property (retain, nonatomic) IBOutlet UILabel *pdescription;
@property (retain, nonatomic) IBOutlet UILabel *pprice;
@property (retain, nonatomic) IBOutlet UISwitch *ppurchased;
@property (nonatomic) BOOL isMaximized;
@property (retain, nonatomic) NSString *productID;

- (id)initWithIdentifier:(NSString*)identifier maximized:(bool)maximized;

+ (CGFloat)getHeight:(BOOL)maximized;

@end
