//
//  UIImageExtension.swift
//  linphone
//
//  Created by Benoît Martins on 06/03/2023.
//

import Foundation

extension UIImage {
	
	public static func withColor(_ color: UIColor, size: CGSize = CGSize(width: 1, height: 1)) -> UIImage {
		let format = UIGraphicsImageRendererFormat()
		format.scale = 1
		let image =  UIGraphicsImageRenderer(size: size, format: format).image { rendererContext in
			color.setFill()
			rendererContext.fill(CGRect(origin: .zero, size: size))
		}
		return image
	}

}
