/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

import Foundation

extension String {
	func localized(comment: String? = nil) -> String {
		return NSLocalizedString(self, comment: comment != nil ? comment! : self)
	}
}

extension String {
	var isOnlyEmojis: Bool {
			let filteredText = self.filter { !$0.isWhitespace }
			return !filteredText.isEmpty && filteredText.allSatisfy { $0.isEmoji }
		}
}

extension Character {
	var isEmoji: Bool {
		guard let scalar = unicodeScalars.first else { return false }
		return scalar.properties.isEmoji && (scalar.value > 0x238C || unicodeScalars.count > 1)
	}
}
