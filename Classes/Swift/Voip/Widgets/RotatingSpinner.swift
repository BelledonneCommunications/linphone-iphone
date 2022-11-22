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

import Foundation

class RotatingSpinner :  UIImageView {
	
	init (color:UIColor = .white) {
		super.init(frame: .zero)
		self.image = UIImage(named: "voip_spinner")
		self.tint(color)
		self.contentMode = .scaleAspectFit
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	func startRotation() {
		let rotation : CABasicAnimation = CABasicAnimation(keyPath: "transform.rotation.z")
		rotation.toValue = NSNumber(value: Double.pi * 2)
		rotation.duration = 2.2
		rotation.isCumulative = true
		rotation.repeatCount = Float.greatestFiniteMagnitude
		self.layer.add(rotation, forKey: "rotationAnimation")
	}
	
	func stopRotation() {
		self.layer.removeAllAnimations()
	}
}

