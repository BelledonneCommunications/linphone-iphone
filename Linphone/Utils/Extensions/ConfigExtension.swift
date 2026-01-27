/*
* Copyright (c) 2010-2023 Belledonne Communications SARL.
*
* This file is part of linphone
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
import linphonesw

// Singleton that manages the Shared Config between app and app extension.

extension Config {
	public func getDouble(section: String, key: String, defaultValue: Double) -> Double {
		if self.hasEntry(section: section, key: key) != 1 {
			return defaultValue
		}
		let stringValue = self.getString(section: section, key: key, defaultString: "")
		return Double(stringValue) ?? defaultValue
	}
	
	public func getString(section: String, key: String) -> String? {
		return hasEntry(section: section, key: key) == 1  ? getString(section: section, key: key, defaultString: "") : nil
	}
	
}
