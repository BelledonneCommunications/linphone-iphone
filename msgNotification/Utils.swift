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
import os
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

			Factory.Instance.logCollectionPath = Factory.Instance.getDataDir(context: UnsafeMutablePointer<Int8>(mutating: (APP_GROUP_ID as NSString).utf8String))
			Factory.Instance.enableLogCollection(state: debugEnabled ? LogCollectionState.Enabled : LogCollectionState.Disabled)
			log.domain = domain
			log.logLevel = debugLevel==0 ? LogLevel.Fatal : LogLevel(rawValue: debugLevel)
			log.addDelegate(delegate: self)
		} else {
			throw LinphoneError.loggingServiceUninitialized
		}
	}
	
	let levelToOSleLogLevel :[Int: OSLogType] =
		[LogLevel.Debug.rawValue:.debug,
		 LogLevel.Trace.rawValue:.info,
		 LogLevel.Message.rawValue:.info,
		 LogLevel.Warning.rawValue:.error,
		 LogLevel.Error.rawValue:.error,
		 LogLevel.Fatal.rawValue:.fault];

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
		if #available(iOS 10.0, *) {
			os_log("%{public}@", type: levelToOSleLogLevel[level.rawValue] ?? .info,message)
		} else {
			NSLog("\(levelStr) [\(domain)] \(message)\n")
		}
	}
}

extension String {
	func getDisplayNameFromSipAddress(lc:Core, logger:LoggingService, groupId:String) -> String? {
		logger.message(message: "looking for display name for \(self)")
		
		
		let defaults = UserDefaults.init(suiteName: groupId)
		let addressBook = defaults?.dictionary(forKey: "addressBook")
		
		if (addressBook == nil) {
			logger.message(message: "address book not found in userDefaults")
			return nil
		}
		
		var usePrefix = true;
		if let account = lc.defaultAccount, let params = account.params {
			usePrefix = params.useInternationalPrefixForCallsAndChats
		}
		
		if let simpleAddr = lc.interpretUrl(url: self, applyInternationalPrefix: usePrefix) {
			simpleAddr.clean()
			let nomalSipaddr = simpleAddr.asString()
			if let displayName = addressBook?[nomalSipaddr] as? String {
				logger.message(message: "display name for \(self): \(displayName)")
				return displayName
			}
		}
		
		logger.message(message: "display name for \(self) not found in userDefaults")
		return nil
	}
}
