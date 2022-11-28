/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


import UIKit
import Foundation
import SnapKit
import linphonesw

class LocalVideoView: UIView {
	
	//Layout constants
	let corner_radius = 15.0
	let aspect_ratio = 4.0/3.0
	let switch_camera_button_margins = 8.0
	let switch_camera_button_size = 30

	var width : CGFloat
	
	var dragZone : UIView? {
		didSet {
			let panGesture = UIPanGestureRecognizer(target: self, action: #selector(drag))
			isUserInteractionEnabled = true
			addGestureRecognizer(panGesture)
		}
	}
		
	let switchCamera = UIImageView(image: UIImage(named:"voip_change_camera")?.tinted(with:.white))

	var callData: CallData? = nil {
		didSet {
			callData?.isRemotelyRecorded.readCurrentAndObserve(onChange: { (isRemotelyRecording) in
				self.isHidden = !(isRemotelyRecording == true)
			})
		}
	}
	
	required init?(coder: NSCoder) {
		width = 0.0
		super.init(coder: coder)
	}
	
	init (width:CGFloat) {
		self.width = width
		super.init(frame: .zero)
		layer.cornerRadius = corner_radius
		clipsToBounds = true
		
		addSubview(switchCamera)
		switchCamera.alignParentTop(withMargin: switch_camera_button_margins).alignParentRight(withMargin: switch_camera_button_margins).square(switch_camera_button_size).done()
		switchCamera.contentMode = .scaleAspectFit
		contentMode = .scaleAspectFill
		
		switchCamera.onClick {
			Core.get().toggleCamera()
		}
		setSizeConstraint()
	}
	
	func getSize() -> CGSize {
		let w = UIDevice.current.orientation.isLandscape ? width*aspect_ratio : width
		let h = !UIDevice.current.orientation.isLandscape ? width*aspect_ratio : width
		return CGSize(width: w,height: h)
	}
	
	func setSizeConstraint() {
		let targetSsize = getSize()
		size(w: targetSsize.width, h: targetSsize.height).done()
	}
	
	func updateSizeConstraint() {
		let targetSsize = getSize()
		updateSize(w: targetSsize.width, h: targetSsize.height).done()
	}
	
	
	@objc func drag(_ sender:UIPanGestureRecognizer){
		dragZone?.bringSubviewToFront(self)
		let translation = sender.translation(in: dragZone)
		center = CGPoint(x: center.x + translation.x, y: center.y + translation.y)
		sender.setTranslation(CGPoint.zero, in: dragZone)
	}
	
}
