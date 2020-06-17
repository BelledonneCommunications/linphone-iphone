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
			super.init()
			let debugLevel = config.getInt(section: "app", key: "debugenable_preference", defaultValue: LogLevel.Debug.rawValue)
			let debugEnabled = (debugLevel >= LogLevel.Debug.rawValue && debugLevel < LogLevel.Error.rawValue)

			if (debugEnabled) {
				Factory.Instance.logCollectionPath = Factory.Instance.getDownloadDir(context: UnsafeMutablePointer<Int8>(mutating: (APP_GROUP_ID as NSString).utf8String))
				Factory.Instance.enableLogCollection(state: LogCollectionState.Enabled)
				log.domain = domain
				log.logLevel = LogLevel(rawValue: debugLevel)
				log.addDelegate(delegate: self)
			}
		} else {
			throw LinphoneError.loggingServiceUninitialized
		}
	}

	override func onLogMessageWritten(logService: LoggingService, domain: String, lev: LogLevel, message: String) {
		let level: String

		switch lev {
		case .Debug:
			level = "Debug"
		case .Trace:
			level = "Trace"
		case .Message:
			level = "Message"
		case .Warning:
			level = "Warning"
		case .Error:
			level = "Error"
		case .Fatal:
			level = "Fatal"
		default:
			level = "unknown"
		}

#if USE_CRASHLYTICS
		Crashlytics.crashlytics().log("\(level) [\(domain)] \(message)\n")
#endif
		NSLog("\(level) [\(domain)] \(message)\n")
	}
}
