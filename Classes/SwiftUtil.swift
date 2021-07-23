//
//  SwiftUtil.swift
//  linphone
//
//  Created by Tof on 23/07/2021.
//

import UIKit

@objc class SwiftUtil: NSObject {

	@objc static func textToImage(drawText text: String, inImage image: UIImage) -> UIImage {
		let textColor = UIColor.black
		let textFont = UIFont(name: "Helvetica", size: 12)!
		let backgroundColor = UIColor.white
		
		let size =  CGSize(width: 120, height: 120)

		let scale = UIScreen.main.scale
		UIGraphicsBeginImageContextWithOptions(size, false, scale)
		let context = UIGraphicsGetCurrentContext()
		backgroundColor.setFill()
		context!.fill(CGRect(x: 0, y: 0, width: size.width, height: size.height))

		let paragraph = NSMutableParagraphStyle()
		paragraph.alignment = .center
		
		let textFontAttributes = [
			NSAttributedString.Key.font: textFont,
			NSAttributedString.Key.foregroundColor: textColor,
			NSAttributedString.Key.paragraphStyle: paragraph,
			] as [NSAttributedString.Key : Any]
		
		image.draw(in: CGRect(origin: CGPoint(x: size.width/2 - (image.size.width)/2,y: 15), size: image.size))

		let rect = CGRect(origin: CGPoint(x: 0,y: 70), size: size)
		text.draw(in: rect, withAttributes: textFontAttributes)

		let newImage = UIGraphicsGetImageFromCurrentImageContext()
		UIGraphicsEndImageContext()

		return newImage!
	}
	
}

