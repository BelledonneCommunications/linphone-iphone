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
	let main100Alpha50: Color
	let main300: Color
	let main500: Color
	let main700: Color
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
		purple.name: purple
	]
	
	static let orange = Theme(
		name: "orange",
		main100: Color(hex: "#FFEACB"),
		main100Alpha50: Color(hex: "#80FFEACB"),
		main300: Color(hex: "#FFB266"),
		main500: Color(hex: "#FF5E00"),
		main700: Color(hex: "#B72D00")
	)
	
	static let yellow = Theme(
		name: "yellow",
		main100: Color(hex: "#FFF5D6"),
		main100Alpha50: Color(hex: "#80FFF5D6"),
		main300: Color(hex: "#FFE799"),
		main500: Color(hex: "#F5BC00"),
		main700: Color(hex: "#A37D00")
	)
	
	static let green = Theme(
		name: "green",
		main100: Color(hex: "#DCF9E7"),
		main100Alpha50: Color(hex: "#80DCF9E7"),
		main300: Color(hex: "#A8F0C2"),
		main500: Color(hex: "#25D366"),
		main700: Color(hex: "#1C9C4B")
	)
	
	static let blue = Theme(
		name: "blue",
		main100: Color(hex: "#D6F4FF"),
		main100Alpha50: Color(hex: "#80D6F4FF"),
		main300: Color(hex: "#99E4FF"),
		main500: Color(hex: "#00AFF0"),
		main700: Color(hex: "#0078A3")
	)
	
	static let red = Theme(
		name: "red",
		main100: Color(hex: "#FBE1DA"),
		main100Alpha50: Color(hex: "#80FBE1DA"),
		main300: Color(hex: "#F5B53A"),
		main500: Color(hex: "#E14318"),
		main700: Color(hex: "#A63211")
	)
	
	static let pink = Theme(
		name: "pink",
		main100: Color(hex: "#FFD6F1"),
		main100Alpha50: Color(hex: "#80FFD6F1"),
		main300: Color(hex: "#FF99DD"),
		main500: Color(hex: "#FF00A9"),
		main700: Color(hex: "#B8007A")
	)
	
	static let purple = Theme(
		name: "purple",
		main100: Color(hex: "#FFD6FF"),
		main100Alpha50: Color(hex: "#80FFD6FF"),
		main300: Color(hex: "#FF99FF"),
		main500: Color(hex: "#800080"),
		main700: Color(hex: "#520052")
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
