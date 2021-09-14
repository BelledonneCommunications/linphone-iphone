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

@objc class SwiftUtil: NSObject {

	@objc static func textToImage(drawText text: String, inImage image: UIImage) -> UIImage {
		let textColor = UIColor.black
		let fontMax = UIFont.systemFont(ofSize: 25)
		let backgroundColor = UIColor.white
		
		let size =  CGSize(width: 120, height: 120)

		let scale = UIScreen.main.scale
		UIGraphicsBeginImageContextWithOptions(size, false, scale)
		let context = UIGraphicsGetCurrentContext()
		backgroundColor.setFill()
		context!.fill(CGRect(x: 0, y: 0, width: size.width, height: size.height))
		
		image.draw(in: CGRect(origin: CGPoint(x: size.width/2 - (image.size.width)/2,y: 15), size: image.size))
		
		let label = UILabel(frame: CGRect(x: 0,y: 0,width: size.width,height: 50))
		label.numberOfLines = 3
		label.font = fontMax
		label.adjustsFontSizeToFitWidth = true
		label.minimumScaleFactor = 0.3
		label.text = text
		label.textColor = textColor
		label.textAlignment = .center
		label.sizeToFit()
		
		imageWithLabel(label: label).draw(in: CGRect(origin: CGPoint(x:0,y: 70), size: CGSize(width: size.width,height: 50)))
	
		let newImage = UIGraphicsGetImageFromCurrentImageContext()
		UIGraphicsEndImageContext()

		return newImage!
	}
	
	static func imageWithLabel(label: UILabel) -> UIImage {
		   UIGraphicsBeginImageContextWithOptions(label.bounds.size, false, 0.0)
		label.layer.render(in: UIGraphicsGetCurrentContext()!)
		let img = UIGraphicsGetImageFromCurrentImageContext()!
		   UIGraphicsEndImageContext()
		   return img
	   }
	
}

