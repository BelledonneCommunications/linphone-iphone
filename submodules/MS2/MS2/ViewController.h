//
//  ViewController.h
//  MS2
//
//  Created by guillaume on 06/05/2015.
//  Copyright (c) 2015 Belldonne Communications. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

- (IBAction)onStartStreamsClick:(id)sender;
- (IBAction)changeCamUp:(id)sender;
- (IBAction)changeCamDown:(id)sender;

@property (weak, nonatomic) IBOutlet UIView *remoteView;
@property (weak, nonatomic) IBOutlet UIView *localView;

@property (weak, nonatomic) IBOutlet UILabel *infoLabel;

@end

