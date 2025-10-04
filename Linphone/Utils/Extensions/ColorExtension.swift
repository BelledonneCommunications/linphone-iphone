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

extension Color {
	
	private static var theme: Theme { ColorProvider.shared.theme }
	
	static let transparentColor = Color(hex: "#00000000")
	static let black = Color(hex: "#000000")
	static let white = Color(hex: "#FFFFFF")
	
	static var orangeMain700: Color { theme.main700 }
	static var orangeMain500: Color { theme.main500 }
	static var orangeMain300: Color { theme.main300 }
	static var orangeMain100: Color { theme.main100 }
	static var orangeMain100Alpha50: Color { theme.main100Alpha50 }
	
	static let grayMain2c800 = Color(hex: "#22334D")
	static let grayMain2c800Alpha65 = Color(hex: "#A622334D")
	static let grayMain2c700 = Color(hex: "#364860")
	static let grayMain2c600 = Color(hex: "#4E6074")
	static let grayMain2c500 = Color(hex: "#6C7A87")
	static let grayMain2c400 = Color(hex: "#9AABB5")
	static let grayMain2c300 = Color(hex: "#C0D1D9")
	static let grayMain2c200 = Color(hex: "#DFECF2")
	static let grayMain2c100 = Color(hex: "#EEF6F8")
	
	static let gray100 = Color(hex: "#F9F9F9")
	static let gray200 = Color(hex: "#EDEDED")
	static let gray300 = Color(hex: "#C9C9C9")
	static let gray400 = Color(hex: "#949494")
	static let gray500 = Color(hex: "#4E4E4E")
	static let gray600 = Color(hex: "#2E3030")
	static let gray900 = Color(hex: "#070707")
	
	static let redDanger200 = Color(hex: "#F5CCBE")
	static let redDanger500 = Color(hex: "#DD5F5F")
	static let redDanger700 = Color(hex: "#9E3548")
	
	static let greenSuccess500 = Color(hex: "#4FAE80")
	static let greenSuccess700 = Color(hex: "#377D71")
	static let greenSuccess200 = Color(hex: "#ACF5C1")
	
	static let blueInfo500 = Color(hex: "#4AA8FF")
	
	static let orangeWarning600 = Color(hex: "#DBB820")
	
	static let orangeAway = Color(hex: "#FFA645")
	
	init(hex: String) {
		let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
		var int: UInt64 = 0
		Scanner(string: hex).scanHexInt64(&int)
		let alpha, red, green, blue: UInt64
		switch hex.count {
		case 3: // RGB (12-bit)
			(alpha, red, green, blue) = (255, (int >> 8) * 17, (int >> 4 & 0xF) * 17, (int & 0xF) * 17)
		case 6: // RGB (24-bit)
			(alpha, red, green, blue) = (255, int >> 16, int >> 8 & 0xFF, int & 0xFF)
		case 8: // ARGB (32-bit)
			(alpha, red, green, blue) = (int >> 24, int >> 16 & 0xFF, int >> 8 & 0xFF, int & 0xFF)
		default:
			(alpha, red, green, blue) = (1, 1, 1, 0)
		}
		
		self.init(
			.sRGB,
			red: Double(red) / 255,
			green: Double(green) / 255,
			blue: Double(blue) / 255,
			opacity: Double(alpha) / 255
		)
	}
}
