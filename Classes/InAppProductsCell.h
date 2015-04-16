//
//  InAppProductsCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 15/04/15.
//
//

#import <UIKit/UIKit.h>

#import "UITransparentTVCell.h"


@interface InAppProductsCell : UITransparentTVCell {

}
@property (retain, nonatomic) IBOutlet UILabel *ptitle;
@property (retain, nonatomic) IBOutlet UILabel *pdescription;
@property (retain, nonatomic) IBOutlet UILabel *pprice;
@property (retain, nonatomic) IBOutlet UISwitch *ppurchased;
@property (nonatomic) BOOL isMaximized;

- (id)initWithIdentifier:(NSString*)identifier;

+ (CGFloat)getHeight:(BOOL)maximized;

@end
