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

// swiftlint:disable large_tuple
extension Int {
	
	public func hmsFrom() -> (Int, Int, Int) {
		return (self / 3600, (self % 3600) / 60, (self % 3600) % 60)
	}
	
	public func convertDurationToString() -> String {
		let (hour, minute, second) = self.hmsFrom()
		let day = hour / 24
		let remainingHour = hour % 24

		if day > 0 && day <= 7 {
			return self.getDay(day: day)
		}

		if hour > 0 {
			return "\(self.getHour(hour: remainingHour))\(self.getMinute(minute: minute))\(self.getSecond(second: second))"
		}

		return "\(self.getMinute(minute: minute))\(self.getSecond(second: second))"
	}

	
	public func formatBytes() -> String {
		let byteCountFormatter = ByteCountFormatter()
		byteCountFormatter.allowedUnits = [.useKB, .useMB, .useGB] // Allows KB, MB and KB
		byteCountFormatter.countStyle = .file // Use file size style
		byteCountFormatter.isAdaptive = true // Adjusts automatically to appropriate unit
		return byteCountFormatter.string(fromByteCount: Int64(self))
	}
	
	private func getDay(day: Int) -> String {
		if day == 1 {
			return NSLocalizedString("conversation_ephemeral_messages_duration_one_day", comment: "")
		}
		let format = NSLocalizedString("conversation_ephemeral_messages_duration_multiple_days", comment: "")
		return String(format: format, day)
	}

	private func getHour(hour: Int) -> String {
		var duration = "\(hour):"
		if hour < 10 {
			duration = "0\(hour):"
		}
		return duration
	}
	
	private func getMinute(minute: Int) -> String {
		if minute == 0 {
			return "00:"
		}

		if minute < 10 {
			return "0\(minute):"
		}

		return "\(minute):"
	}
	
	private func getSecond(second: Int) -> String {
		if second == 0 {
			return "00"
		}

		if second < 10 {
			return "0\(second)"
		}
		return "\(second)"
	}
}

// swiftlint:enable large_tuple
