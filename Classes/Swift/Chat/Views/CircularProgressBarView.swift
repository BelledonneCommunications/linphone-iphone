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

class CircularProgressBarView: UIView {
	
	private var circleLayer = CAShapeLayer()
	private var progressLayer = CAShapeLayer()
	
	private var startPoint = CGFloat(-Double.pi / 2)
 	private var endPoint = CGFloat(3 * Double.pi / 2)
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		createCircularPath()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	func createCircularPath() {
		let circularPath = UIBezierPath(arcCenter: CGPoint(x: 69, y: 69), radius: 20, startAngle: startPoint, endAngle: endPoint, clockwise: true)
		circleLayer.path = circularPath.cgPath
		circleLayer.fillColor = UIColor.clear.cgColor
		circleLayer.lineCap = .round
		circleLayer.lineWidth = 10.0
		circleLayer.strokeEnd = 1.0
		circleLayer.strokeColor = VoipTheme.backgroundWhiteBlack.get().cgColor
		layer.addSublayer(circleLayer)
		progressLayer.path = circularPath.cgPath
		progressLayer.fillColor = VoipTheme.backgroundWhiteBlack.get().cgColor
		progressLayer.lineCap = .round
		progressLayer.lineWidth = 5.0
		progressLayer.strokeEnd = 0
		progressLayer.strokeColor = VoipTheme.primary_color.cgColor
		layer.addSublayer(progressLayer)
	}
	
	func progressAnimation(fromValue: Float, toValue: Float) {
		let circularProgressAnimation = CABasicAnimation(keyPath: "strokeEnd")
		circularProgressAnimation.duration = 0
		circularProgressAnimation.fromValue = fromValue
		circularProgressAnimation.toValue = toValue
		circularProgressAnimation.fillMode = .forwards
		circularProgressAnimation.isRemovedOnCompletion = false
		progressLayer.add(circularProgressAnimation, forKey: "progressAnim")
	}
}
