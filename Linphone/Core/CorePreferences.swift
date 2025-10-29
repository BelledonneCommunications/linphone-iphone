/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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
import linphonesw

class CorePreferences {
	static var printLogsInLogcat: Bool {
		get {
			return Config.get().getBool(section: "app", key: "debug", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "app", key: "debug", value: newValue)
		}
	}

	static var firstLaunch: Bool {
		get {
			return Config.get().getBool(section: "app", key: "first_6.0_launch", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "app", key: "first_6.0_launch", value: newValue)
		}
	}
	
	static var linphoneConfigurationVersion: Int {
		get {
			return Config.get().getInt(section: "app", key: "config_version", defaultValue: 52005)
		}
		set {
			Config.get().setInt(section: "app", key: "config_version", value: newValue)
		}
	}
	
	static var checkForUpdateServerUrl: String {
		get {
			let raw = Config.get().getString(section: "misc", key: "version_check_url_root", defaultString: "")
			return safeString(raw, defaultValue: "")
		}
		set {
			Config.get().setString(section: "misc", key: "version_check_url_root", value: newValue)
		}
	}
	
	static var conditionsAndPrivacyPolicyAccepted: Bool {
		get {
			return Config.get().getBool(section: "app", key: "read_and_agree_terms_and_privacy", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "app", key: "read_and_agree_terms_and_privacy", value: newValue)
		}
	}
	
	static var publishPresence: Bool {
		get {
			return Config.get().getBool(section: "app", key: "publish_presence", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "app", key: "publish_presence", value: newValue)
		}
	}
	
	static var keepServiceAlive: Bool {
		get {
			return Config.get().getBool(section: "app", key: "keep_service_alive", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "app", key: "keep_service_alive", value: newValue)
		}
	}
	
	static var deviceName: String {
		get {
			let raw = Config.get().getString(section: "app", key: "device", defaultString: "").trimmingCharacters(in: .whitespaces)
			return safeString(raw, defaultValue: "")
		}
		set {
			Config.get().setString(section: "app", key: "device", value: newValue.trimmingCharacters(in: .whitespaces))
		}
	}
	
	static var routeAudioToSpeakerWhenVideoIsEnabled: Bool {
		get {
			return Config.get().getBool(section: "app", key: "route_audio_to_speaker_when_video_enabled", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "app", key: "route_audio_to_speaker_when_video_enabled", value: newValue)
		}
	}
	
	static var automaticallyStartCallRecording: Bool {
		get {
			return Config.get().getBool(section: "app", key: "auto_start_call_record", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "app", key: "auto_start_call_record", value: newValue)
		}
	}
	
	static var showDialogWhenCallingDeviceUuidDirectly: Bool {
		get {
			return Config.get().getBool(section: "app", key: "show_confirmation_dialog_zrtp_trust_call", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "app", key: "show_confirmation_dialog_zrtp_trust_call", value: newValue)
		}
	}
	
	static var markConversationAsReadWhenDismissingMessageNotification: Bool {
		get {
			return Config.get().getBool(section: "app", key: "mark_as_read_notif_dismissal", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "app", key: "mark_as_read_notif_dismissal", value: newValue)
		}
	}
	
	static var contactsFilter: String {
		get {
			let raw = Config.get().getString(section: "ui", key: "contacts_filter", defaultString: "")
			return safeString(raw, defaultValue: "")
		}
		set {
			Config.get().setString(section: "ui", key: "contacts_filter", value: newValue)
		}
	}
	
	static var showFavoriteContacts: Bool {
		get {
			return Config.get().getBool(section: "ui", key: "show_favorites_contacts", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "ui", key: "show_favorites_contacts", value: newValue)
		}
	}
	
	static var friendListInWhichStoreNewlyCreatedFriends: String {
		get {
			return Config.get().getString(section: "app", key: "friend_list_to_store_newly_created_contacts", defaultString: "Linphone address-book")
		}
		set {
			Config.get().setString(section: "app", key: "friend_list_to_store_newly_created_contacts", value: newValue)
		}
	}
	
	static var voiceRecordingMaxDuration: Int {
		get {
			return Config.get().getInt(section: "app", key: "voice_recording_max_duration", defaultValue: 600000)
		}
		set {
			Config.get().setInt(section: "app", key: "voice_recording_max_duration", value: newValue)
		}
	}
	
	static var darkMode: Int {
		get {
			if !darkModeAllowed { return 0 }
			return Config.get().getInt(section: "app", key: "dark_mode", defaultValue: -1)
		}
		set {
			Config.get().setInt(section: "app", key: "dark_mode", value: newValue)
		}
	}
	
	static var enableSecureMode: Bool {
		get {
			return Config.get().getBool(section: "ui", key: "enable_secure_mode", defaultValue: true)
		}
		set {
			Config.get().setBool(section: "ui", key: "enable_secure_mode", value: newValue)
		}
	}
	
	static var themeMainColor: String {
		get {
			let raw = Config.get().getString(section: "ui", key: "theme_main_color", defaultString: "orange")
			return safeString(raw, defaultValue: "orange")
		}
		set {
			Config.get().setString(section: "ui", key: "theme_main_color", value: newValue)
		}
	}
	
	static var themeAboutPictureUrl: String? {
		get {
			return Config.get().getString(section: "ui", key: "theme_about_picture_url", defaultString: nil)
		}
	}
	
	static var darkModeAllowed: Bool {
		return Config.get().getBool(section: "ui", key: "dark_mode_allowed", defaultValue: true)
	}
	
	static var changeMainColorAllowed: Bool {
		return Config.get().getBool(section: "ui", key: "change_main_color_allowed", defaultValue: false)
	}
	
	static var hideSettings: Bool {
		return Config.get().getBool(section: "ui", key: "hide_settings", defaultValue: false)
	}
	
	static var maxAccountsCount: Int {
		return Config.get().getInt(section: "ui", key: "max_account", defaultValue: 0)
	}
	
	/*
	static var configPath: String {
		return context.view.window?.rootViewController?.view.frame.origin.x ?? "" + "/.linphonerc"
	}
	
	static var factoryConfigPath: String {
		return context.view.window?.rootViewController?.view.frame.origin.x ?? "" + "/linphonerc"
	}
	
	func copyAssetsFromPackage() {
		copy(from: "linphonerc_default", to: configPath)
		copy(from: "linphonerc_factory", to: factoryConfigPath, overrideIfExists: true)
	}
	*/
	
	static var vfsEnabled: Bool {
		get {
			return Config.get().getBool(section: "app", key: "vfs_enabled", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "app", key: "vfs_enabled", value: newValue)
		}
	}
	
	static var acceptEarlyMedia: Bool {
		get {
			return Config.get().getBool(section: "sip", key: "incoming_calls_early_media", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "sip", key: "incoming_calls_early_media", value: newValue)
		}
	}
	
	static var allowOutgoingEarlyMedia: Bool {
		get {
			return Config.get().getBool(section: "sip", key: "real_early_media", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "sip", key: "real_early_media", value: newValue)
		}
	}
	
	static var defaultDomain: String {
		get {
			let raw = Config.get().getString(section: "app", key: "default_domain", defaultString: "sip.linphone.org")
			return safeString(raw, defaultValue: "sip.linphone.org")
		}
		set {
			Config.get().setString(section: "app", key: "default_domain", value: newValue)
		}
	}
    
	static var disableChatFeature: Bool {
		get {
			return Config.get().getBool(section: "ui", key: "disable_chat_feature", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "ui", key: "disable_chat_feature", value: newValue)
		}
	}
	
	static var disableMeetings: Bool {
		get {
			return Config.get().getBool(section: "ui", key: "disable_meetings_feature", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "ui", key: "disable_meetings_feature", value: newValue)
		}
	}
	
	static var hideSipAddresses: Bool {
		get {
			return Config.get().getBool(section: "ui", key: "hide_sip_addresses", defaultValue: false)
		}
		set {
			Config.get().setBool(section: "ui", key: "hide_sip_addresses", value: newValue)
		}
	}
	
	private func copy(from: String, to: String, overrideIfExists: Bool = false) {
		let fileManager = FileManager.default
		if fileManager.fileExists(atPath: to), !overrideIfExists {
			return
		}
		
		if let assetPath = Bundle.main.path(forResource: from, ofType: "") {
			do {
				try fileManager.copyItem(atPath: assetPath, toPath: to)
			} catch {
				print("Error copying file: \(error)")
			}
		}
	}
	
	private static func safeString(_ raw: String?, defaultValue: String = "") -> String {
		guard let raw = raw else { return defaultValue }
		if let data = raw.data(using: .utf8) {
			return String(decoding: data, as: UTF8.self)
		}
		return defaultValue
	}
}
