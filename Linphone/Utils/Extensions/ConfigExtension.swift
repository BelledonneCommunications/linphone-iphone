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

	private static var _instance: Config?

	public func getDouble(section: String, key: String, defaultValue: Double) -> Double {
		if self.hasEntry(section: section, key: key) != 1 {
			return defaultValue
		}
		let stringValue = self.getString(section: section, key: key, defaultString: "")
		return Double(stringValue) ?? defaultValue
	}

	public static func get() -> Config {
		if _instance == nil {
			let factoryPath = FileUtil.bundleFilePath("linphonerc-factory")!
			_instance =  Config.newForSharedCore(appGroupId: Config.appGroupName, configFilename: "linphonerc", factoryConfigFilename: factoryPath)!
		}
		return _instance!
	}
	
	public func getString(section: String, key: String) -> String? {
		return hasEntry(section: section, key: key) == 1  ? getString(section: section, key: key, defaultString: "") : nil
	}
	
	static let appGroupName = "group.org.linphone.phone.msgNotification"
	// Needs to be the same name in App Group (capabilities in ALL targets - app & extensions - content + service), can't be stored in the Config itself the Config needs this value to get created
	static let teamID = Config.get().getString(section: "app", key: "team_id", defaultString: "")
	static let earlymediaContentExtCatIdentifier = Config.get().getString(section: "app", key: "extension_category", defaultString: "")
	
	// Default values in app
	static let serveraddress =  Config.get().getString(section: "app", key: "server", defaultString: "")
	static let defaultUsername =  Config.get().getString(section: "app", key: "user", defaultString: "")
	static let defaultPass =  Config.get().getString(section: "app", key: "pass", defaultString: "")
	
	static let pushNotificationsInterval =  Config.get().getInt(section: "net", key: "pn-call-remote-push-interval", defaultValue: 3)
	
	static let voiceRecordingMaxDuration =  Config.get().getInt(section: "app", key: "voice_recording_max_duration", defaultValue: 600000)

}
