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
import Photos
import linphonesw


@objc class SwiftUtil: NSObject {
	
	@objc static func textToImage(drawText text: String, inImage image: UIImage, forReplyBubble:Bool) -> UIImage {
		let textColor = UIColor.black
		let fontMax = UIFont.systemFont(ofSize: 12)
		let backgroundColor = forReplyBubble ? UIColor(red: 246/255.0, green: 246/255.0, blue: 246/255.0, alpha: 1.0) : UIColor.white
		let size =  CGSize(width: 120, height: 120)
		let scale = UIScreen.main.scale
		UIGraphicsBeginImageContextWithOptions(size, false, scale)
		let context = UIGraphicsGetCurrentContext()
		backgroundColor.setFill()
		context!.fill(CGRect(x: 0, y: 0, width: size.width, height: size.height))
		let imageSize:CGSize = forReplyBubble ? CGSize(width: 80, height:80*(image.size.height / image.size.width)): image.size
		image.draw(in: CGRect(origin: CGPoint(x: size.width/2 - (imageSize.width)/2,y: (forReplyBubble ? size.height/2 : 90/2) - (imageSize.height)/2), size: imageSize))

		if (!forReplyBubble) {
			let label = UILabel(frame: CGRect(x: 0,y: 0,width: size.width,height: 30))
			label.numberOfLines = 1
			label.font = fontMax
			label.adjustsFontSizeToFitWidth = false
			label.text = text
			label.textColor = textColor
			label.textAlignment = .center
			label.allowsDefaultTighteningForTruncation = true
			label.lineBreakMode = .byTruncatingMiddle
			imageWithLabel(label: label).draw(in: CGRect(origin: CGPoint(x:5,y: 70), size: CGSize(width: size.width-10,height: 30)))
		} else {
			let borderWidth: CGFloat = 2.0
			let path = UIBezierPath(roundedRect: CGRect(x: 0, y: 0, width: 120, height: 120).insetBy(dx: borderWidth / 2, dy: borderWidth / 2), cornerRadius: 5.0)
			context!.saveGState()
			path.addClip()
			UIColor.gray.setStroke()
			path.lineWidth = borderWidth
			path.stroke()
		}
		
		let newImage = UIGraphicsGetImageFromCurrentImageContext()
		UIGraphicsEndImageContext()
		
		return newImage!
	}
	
	static func imageWithLabel(label: UILabel) -> UIImage {
		UIGraphicsBeginImageContextWithOptions(label.frame.size, false, 0.0)
		label.layer.render(in: UIGraphicsGetCurrentContext()!)
		let img = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		return img
	}
	
	// Image cache
	static var imageCache:[String:UIImage] = [:]
	
	@objc static func getCachedImage(key:String) -> UIImage? {
		return key != nil ? imageCache[key] : nil
	}
	
	@objc static func setCachedImage(key:String,image:UIImage)  {
		imageCache[key] = image
	}

	@objc static func resetCachedAsset()  {
		imageCache.removeAll()
	}
	
	// Chat bubble height cache :
	
	static var cacheMessageSize:[String:CGSize] = [:]
	
	@objc static func getCachedMessageHeight(cmessage:OpaquePointer) -> CGSize {
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		if let cached = cacheMessageSize[message.messageId] {
			return cached
		} else {
			return .zero
		}
	}
	
	@objc static func setCachedMessageHeight(cmessage:OpaquePointer, size:CGSize) {
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		cacheMessageSize[message.messageId] = size
	}
	
	@objc static func removeCachedMessageHeight(cmessage:OpaquePointer) {
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		cacheMessageSize.removeValue(forKey: message.messageId)
	}
	
	@objc static func messageHeightCanBeCached(cmessage:OpaquePointer) -> Bool {
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		return  (message.isOutgoing && [.Delivered, .DeliveredToUser, .Displayed].contains(message.state)) ||  (!message.isOutgoing && [.Displayed].contains(message.state))
	}
	
	
}

