//
//  UIImageViewDeletable.m
//  linphone
//
//  Created by benjamin_verdier on 28/06/2018.
//

#import "UIImageViewDeletable.h"

@interface UIImageViewDeletable ()

@end

@implementation UIImageViewDeletable

- (UIImageViewDeletable *)init {
    self = [super init];
    if (self) {
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        // resize cell to match .nib size. It is needed when resized the cell to
        // correctly adapt its height too
        UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:arrayOfViews.count - 1]);
        [self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
        [self addSubview:sub];
    }
    return self;
}


- (UIImageViewDeletable *)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        NSArray *arrayOfViews =
        [[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
        // resize cell to match .nib size. It is needed when resized the cell to
        // correctly adapt its height too
        UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:arrayOfViews.count - 1]);
        [self setFrame:frame];
        [self addSubview:sub];
    }
    return self;
}

- (IBAction)onDeletePressed {
    [_deleteDelegate deleteImageWithAssetId:_assetId];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
