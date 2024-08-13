//
//  TimeZoneExtension.swift
//  Linphone
//
//  Created by QuentinArguillere on 06/08/2024.
//

import Foundation

extension TimeZone {
	// Format timezone identifier as a string of the form : GMT{+,-}[-12, 12]:00 - {Identifier}
	func formattedString() -> String {
		let gmtOffset = self.secondsFromGMT()/3600
		return "GMT\(gmtOffset >= 0 ? "+" : "")\(gmtOffset):00 - \(self.identifier)"
	}
}
