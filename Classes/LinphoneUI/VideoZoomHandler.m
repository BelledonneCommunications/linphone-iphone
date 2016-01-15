/* VideoZoomHandler.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "VideoZoomHandler.h"
#include "linphone/linphonecore.h"
#import "LinphoneManager.h"

@implementation VideoZoomHandler

- (void)zoomInOut:(UITapGestureRecognizer *)reco {
	if (zoomLevel != 1)
		zoomLevel = 1;
	else
		zoomLevel = 2;

	if (zoomLevel != 1) {
		CGPoint point = [reco locationInView:videoView];
		cx = point.x / videoView.frame.size.width;
		cy = 1 - point.y / videoView.frame.size.height;
	} else {
		cx = cy = 0.5;
	}
	linphone_call_zoom_video(linphone_core_get_current_call(LC), zoomLevel, &cx, &cy);
}

- (void)videoPan:(UIPanGestureRecognizer *)reco {
	if (zoomLevel <= 1.0)
		return;

	float x, y;
	CGPoint translation = [reco translationInView:videoView];
	if ([reco state] == UIGestureRecognizerStateEnded) {
		cx -= translation.x / videoView.frame.size.width;
		cy += translation.y / videoView.frame.size.height;
		x = cx;
		y = cy;
	} else if ([reco state] == UIGestureRecognizerStateChanged) {
		x = cx - translation.x / videoView.frame.size.width;
		y = cy + translation.y / videoView.frame.size.height;
		[reco setTranslation:CGPointMake(0, 0) inView:videoView];
	} else {
		return;
	}

	linphone_call_zoom_video(linphone_core_get_current_call(LC), zoomLevel, &x, &y);
	cx = x;
	cy = y;
}

- (void)pinch:(UIPinchGestureRecognizer *)reco {
	float s = zoomLevel;
	// CGPoint point = [reco locationInView:videoGroup];
	// float ccx = cx + (point.x / videoGroup.frame.size.width - 0.5) / s;
	// float ccy = cy - (point.y / videoGroup.frame.size.height - 0.5) / s;
	if ([reco state] == UIGestureRecognizerStateEnded) {
		zoomLevel = MAX(MIN(zoomLevel * reco.scale, 3.0), 1.0);
		s = zoomLevel;
		// cx = ccx;
		// cy = ccy;
	} else if ([reco state] == UIGestureRecognizerStateChanged) {
		s = zoomLevel * reco.scale;
		s = MAX(MIN(s, 3.0), 1.0);
	} else if ([reco state] == UIGestureRecognizerStateBegan) {

	} else {
		return;
	}

	linphone_call_zoom_video(linphone_core_get_current_call(LC), s, &cx, &cy);
}

- (void)resetZoom {
	zoomLevel = 1;
	cx = cy = 0.5;
}

- (void)setup:(UIView *)view {
	videoView = view;

	UITapGestureRecognizer *doubleFingerTap =
		[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(zoomInOut:)];
	[doubleFingerTap setNumberOfTapsRequired:2];
	[doubleFingerTap setNumberOfTouchesRequired:1];
	[videoView addGestureRecognizer:doubleFingerTap];

	UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(videoPan:)];
	[videoView addGestureRecognizer:pan];
	UIPinchGestureRecognizer *pinchReco =
		[[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(pinch:)];
	[videoView addGestureRecognizer:pinchReco];

	[self resetZoom];
}

@end
