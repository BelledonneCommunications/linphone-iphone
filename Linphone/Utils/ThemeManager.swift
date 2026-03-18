/*
 * Copyright (c) 2010-2025 Belledonne Communications SARL.
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

import SwiftUI
import Combine

// MARK: - Theme definition

struct Theme: Equatable {
	let name: String
	let main100: Color
	let main500: Color
}

// MARK: - Theme Manager

final class ThemeManager: ObservableObject {
	static let shared = ThemeManager()
	private let themeKey = "selectedTheme"
	
	@Published var currentTheme: Theme = ThemeManager.orange
	
	private init() {
		let storedName = UserDefaults.standard.string(forKey: themeKey)
		currentTheme = themes[storedName ?? ""] ?? ThemeManager.orange
	}
	
	func applyTheme(named name: String) {
		guard let theme = themes[name] else { return }
		withAnimation(.easeInOut(duration: 0.3)) {
			self.currentTheme = theme
		}
		UserDefaults.standard.setValue(name, forKey: themeKey)
	}
	
	// MARK: - Theme Presets
	
	let themes: [String: Theme] = [
		orange.name: orange,
		yellow.name: yellow,
		green.name: green,
		blue.name: blue,
		red.name: red,
		pink.name: pink,
		purple.name: purple,
		terracotta.name: terracotta,
		lavender.name: lavender,
		honey.name: honey,
		burgundy.name: burgundy,
		mint.name: mint,
		coral.name: coral,
		plum.name: plum,
		titanium.name: titanium,
		mineral_blue.name: mineral_blue
	]
	
	static let orange = Theme(
		name: "orange",
		main100: Color(hex: "#FFEACB"),
		main500: Color(hex: "#FF5E00")
	)
	
	static let yellow = Theme(
		name: "yellow",
		main100: Color(hex: "#FFF5D6"),
		main500: Color(hex: "#F5BC00")
	)
	
	static let green = Theme(
		name: "green",
		main100: Color(hex: "#DCF9E7"),
		main500: Color(hex: "#25D366")
	)
	
	static let blue = Theme(
		name: "blue",
		main100: Color(hex: "#D6F4FF"),
		main500: Color(hex: "#00AFF0")
	)
	
	static let red = Theme(
		name: "red",
		main100: Color(hex: "#FBE1DA"),
		main500: Color(hex: "#E14318")
	)
	
	static let pink = Theme(
		name: "pink",
		main100: Color(hex: "#FFD6F1"),
		main500: Color(hex: "#FF00A9")
	)
	
	static let purple = Theme(
		name: "purple",
		main100: Color(hex: "#FFD6FF"),
		main500: Color(hex: "#800080")
	)
	
	static let terracotta = Theme(
		name: "terracotta",
		main100: Color(hex: "#F2D2C7"),
		main500: Color(hex: "#C86B45")
	)
	
	static let lavender = Theme(
		name: "lavender",
		main100: Color(hex: "#E9E7F2"),
		main500: Color(hex: "#7F79B5")
	)
	
	static let honey = Theme(
		name: "honey",
		main100: Color(hex: "#FBFCE9"),
		main500: Color(hex: "#D9A441")
	)
	
	static let burgundy = Theme(
		name: "burgundy",
		main100: Color(hex: "#F6BBC3"),
		main500: Color(hex: "#7A1E39")
	)
	
	static let mint = Theme(
		name: "mint",
		main100: Color(hex: "#DEF2E8"),
		main500: Color(hex: "#7AC9A1")
	)
	
	static let coral = Theme(
		name: "coral",
		main100: Color(hex: "#FFD3D6"),
		main500: Color(hex: "#F26B5E")
	)
	
	static let plum = Theme(
		name: "plum",
		main100: Color(hex: "#DBBED9"),
		main500: Color(hex: "#9B4F96")
	)
	
	static let titanium = Theme(
		name: "titanium",
		main100: Color(hex: "#D9D9D9"),
		main500: Color(hex: "#8A939B")
	)
	
	static let mineral_blue = Theme(
		name: "mineral_blue",
		main100: Color(hex: "#C5E0F3"),
		main500: Color(hex: "#669ED7")
	)
}

// MARK: - Color Provider (reactive bridge for SwiftUI)

final class ColorProvider: ObservableObject {
	static let shared = ColorProvider()
	
	@Published private(set) var theme: Theme
	private var cancellable: AnyCancellable?
	
	private init() {
		let manager = ThemeManager.shared
		self.theme = manager.currentTheme
		
		cancellable = manager.$currentTheme
			.sink { [weak self] theme in
				withAnimation(.easeInOut(duration: 0.3)) {
					self?.theme = theme
				}
			}
	}
}
