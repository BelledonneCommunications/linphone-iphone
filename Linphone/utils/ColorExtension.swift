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
	
	static let transparent_color = Color(hex: "#00000000")
	static let black = Color(hex: "#000000")
	static let white = Color(hex: "#FFFFFF")

	static let orange_main_700 = Color(hex: "#B72D00")
	static let orange_main_500 = Color(hex: "#FF5E00")
	static let orange_main_300 = Color(hex: "#FFB266")
	static let orange_main_100 = Color(hex: "#FFEACB")
	static let orange_main_100_alpha_50 = Color(hex: "#80FFEACB")

	static let gray_main2_800 = Color(hex: "#22334D")
	static let gray_main2_800_alpha_65 = Color(hex: "#A622334D")
	static let gray_main2_700 = Color(hex: "#364860")
	static let gray_main2_600 = Color(hex: "#4E6074")
	static let gray_main2_500 = Color(hex: "#6C7A87")
	static let gray_main2_400 = Color(hex: "#9AABB5")
	static let gray_main2_300 = Color(hex: "#C0D1D9")
	static let gray_main2_200 = Color(hex: "#DFECF2")
	static let gray_main2_100 = Color(hex: "#EEF6F8")

	static let gray_100 = Color(hex: "#F9F9F9")
	static let gray_200 = Color(hex: "#EDEDED")
	static let gray_300 = Color(hex: "#C9C9C9")
	static let gray_400 = Color(hex: "#949494")
	static let gray_500 = Color(hex: "#4E4E4E")
	static let gray_600 = Color(hex: "#2E3030")
	static let gray_900 = Color(hex: "#070707")

	static let red_danger_200 = Color(hex: "#F5CCBE")
	static let red_danger_500 = Color(hex: "#DD5F5F")
	static let red_danger_700 = Color(hex: "#9E3548")

	static let green_success_500 = Color(hex: "#4FAE80")
	static let green_success_700 = Color(hex: "#377D71")
	static let green_success_200 = Color(hex: "#ACF5C1")

	static let blue_info_500 = Color(hex: "#4AA8FF")

	static let orange_warning_600 = Color(hex: "#DBB820")

	static let orange_away = Color(hex: "#FFA645")
	
	init(hex: String) {
		let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
		var int: UInt64 = 0
		Scanner(string: hex).scanHexInt64(&int)
		let a, r, g, b: UInt64
		switch hex.count {
		case 3: // RGB (12-bit)
			(a, r, g, b) = (255, (int >> 8) * 17, (int >> 4 & 0xF) * 17, (int & 0xF) * 17)
		case 6: // RGB (24-bit)
			(a, r, g, b) = (255, int >> 16, int >> 8 & 0xFF, int & 0xFF)
		case 8: // ARGB (32-bit)
			(a, r, g, b) = (int >> 24, int >> 16 & 0xFF, int >> 8 & 0xFF, int & 0xFF)
		default:
			(a, r, g, b) = (1, 1, 1, 0)
		}

		self.init(
			.sRGB,
			red: Double(r) / 255,
			green: Double(g) / 255,
			blue:  Double(b) / 255,
			opacity: Double(a) / 255
		)
	}
}
