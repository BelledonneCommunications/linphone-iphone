/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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
import SwiftUI

let cssWeightToFontWeightName = [
	100: "UltraLight",
	200: "Thin",
	300: "Light",
	400: "Regular",
	500: "Medium",
	600: "SemiBold",
	700: "Bold",
	800: "Heavy",
	900: "Black"
]

extension View {
	
	func default_text_style_300(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Light", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func default_text_style(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Regular", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func default_text_style_500(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Medium", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func default_text_style_600(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-SemiBold", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func default_text_style_700(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Bold", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func default_text_style_800(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-ExtraBold", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func default_text_style_white_300(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Light", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func default_text_style_white(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Regular", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func default_text_style_white_500(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Medium", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func default_text_style_white_600(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-SemiBold", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func default_text_style_white_700(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Bold", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func default_text_style_white_800(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-ExtraBold", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func default_text_style_orange_300(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Light", size: styleSize))
			.foregroundStyle(Color.orangeMain500)
	}
	
	func default_text_style_orange(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Regular", size: styleSize))
			.foregroundStyle(Color.orangeMain500)
	}
	
	func default_text_style_orange_500(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Medium", size: styleSize))
			.foregroundStyle(Color.orangeMain500)
	}
	
	func default_text_style_orange_600(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-SemiBold", size: styleSize))
			.foregroundStyle(Color.orangeMain500)
	}
	
	func default_text_style_orange_700(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Bold", size: styleSize))
			.foregroundStyle(Color.orangeMain500)
	}
	
	func default_text_style_orange_800(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-ExtraBold", size: styleSize))
			.foregroundStyle(Color.orangeMain500)
	}
	
	func welcome_text_style_white_800(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-ExtraBold", size: styleSize))
			.foregroundStyle(Color.white)
	}
	
	func welcome_text_style_gray_800(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-ExtraBold", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func welcome_text_style_gray(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Regular", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
	
	func profile_mode_text_style_gray_800(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-ExtraBold", size: styleSize))
			.foregroundStyle(Color.gray900)
	}
	
	func profile_mode_text_style_gray(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Regular", size: styleSize))
			.foregroundStyle(Color.grayMain2c600)
	}
    
    func contact_text_style_500(styleSize: CGFloat) -> some View {
        self.font(Font.custom("NotoSans-Medium", size: styleSize))
            .foregroundStyle(Color.grayMain2c400)
    }
	
	func default_text_style_grey_400(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans", size: styleSize))
			.foregroundStyle(Color.grayMain2c700)
	}
	
	func default_text_style_uncolored(styleSize: CGFloat) -> some View {
		self.font(Font.custom("NotoSans-Light", size: styleSize))
	}
	
	func text_style(fontSize: CGFloat, fontWeight: Int, fontColor: Color) -> some View {
		return self.font(Font.custom("NotoSans-"+cssWeightToFontWeightName[fontWeight]!, size: fontSize))
			.foregroundStyle(fontColor)
	}
}
