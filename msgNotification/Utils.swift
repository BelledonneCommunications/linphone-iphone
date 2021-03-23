/*
* Copyright (c) 2010-2020 Belledonne Communications SARL.
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

import linphonesw
#if USE_CRASHLYTICS
import Firebase
#endif

enum LinphoneError: Error {
	case timeout
	case loggingServiceUninitialized
}

class LinphoneLoggingServiceManager: LoggingServiceDelegate {
	init(config: Config, log: LoggingService?, domain: String) throws {
		if let log = log {
			let debugLevel = config.getInt(section: "app", key: "debugenable_preference", defaultValue: LogLevel.Debug.rawValue)
			let debugEnabled = (debugLevel >= LogLevel.Debug.rawValue && debugLevel < LogLevel.Error.rawValue)

			Factory.Instance.logCollectionPath = Factory.Instance.getDownloadDir(context: UnsafeMutablePointer<Int8>(mutating: (APP_GROUP_ID as NSString).utf8String))
			Factory.Instance.enableLogCollection(state: debugEnabled ? LogCollectionState.Enabled : LogCollectionState.Disabled)
			log.domain = domain
			log.logLevel = debugLevel==0 ? LogLevel.Fatal : LogLevel(rawValue: debugLevel)
			log.addDelegate(delegate: self)
		} else {
			throw LinphoneError.loggingServiceUninitialized
		}
	}

	func onLogMessageWritten(logService: LoggingService, domain: String, level: LogLevel, message: String) {
		let levelStr: String

		switch level {
		case .Debug:
			levelStr = "Debug"
		case .Trace:
			levelStr = "Trace"
		case .Message:
			levelStr = "Message"
		case .Warning:
			levelStr = "Warning"
		case .Error:
			levelStr = "Error"
		case .Fatal:
			levelStr = "Fatal"
		default:
			levelStr = "unknown"
		}

#if USE_CRASHLYTICS
		Crashlytics.crashlytics().log("\(levelStr) [\(domain)] \(message)\n")
#endif
		NSLog("\(levelStr) [\(domain)] \(message)\n")
	}
}
