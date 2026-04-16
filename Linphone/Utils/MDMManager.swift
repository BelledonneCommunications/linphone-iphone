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
import CryptoKit
import linphonesw

class MDMManager {

	static let shared = MDMManager()

	static let configurationRemovedNotification = Notification.Name("MDMManager.configurationRemoved")
	static let configurationAppliedNotification = Notification.Name("MDMManager.configurationApplied")

	private static let hasMDMConfigKey = "MDMManager.hasMDMConfig"
	private static let lastXMLConfigSHA256Key = "MDMManager.lastXMLConfigSHA256"
	private static let lastCoreConfigSHA256Key = "MDMManager.lastCoreConfigSHA256"

	private var hasMDMConfig: Bool {
		get { UserDefaults.standard.bool(forKey: MDMManager.hasMDMConfigKey) }
		set {
			guard UserDefaults.standard.bool(forKey: MDMManager.hasMDMConfigKey) != newValue else { return }
			UserDefaults.standard.set(newValue, forKey: MDMManager.hasMDMConfigKey)
		}
	}

	private var lastXMLConfigSHA256: String? {
		get { UserDefaults.standard.string(forKey: MDMManager.lastXMLConfigSHA256Key) }
		set {
			guard UserDefaults.standard.string(forKey: MDMManager.lastXMLConfigSHA256Key) != newValue else { return }
			UserDefaults.standard.set(newValue, forKey: MDMManager.lastXMLConfigSHA256Key)
		}
	}

	private var lastCoreConfigSHA256: String? {
		get { UserDefaults.standard.string(forKey: MDMManager.lastCoreConfigSHA256Key) }
		set {
			guard UserDefaults.standard.string(forKey: MDMManager.lastCoreConfigSHA256Key) != newValue else { return }
			UserDefaults.standard.set(newValue, forKey: MDMManager.lastCoreConfigSHA256Key)
		}
	}

	private var isApplyingConfig = false

	private var lastSeenManagedConfigSignature: String?

	func managedConfigChangedSinceLastCheck() -> Bool {
		let signature: String
		if let mdmConfig = UserDefaults.standard.dictionary(forKey: "com.apple.configuration.managed") {
			signature = sha256Hash(of: mdmConfig)
		} else {
			signature = ""
		}
		if signature == lastSeenManagedConfigSignature { return false }
		lastSeenManagedConfigSignature = signature
		return true
	}

	func loadXMLConfigFromMdm(config: Config) {
		guard let mdmConfig = UserDefaults.standard.dictionary(forKey: "com.apple.configuration.managed"),
			  let xmlConfig = mdmConfig["xml-config"] as? String else {
			lastXMLConfigSHA256 = nil
			return
		}

		let hash = sha256Hash(of: ["xml-config": xmlConfig])
		if hash == lastXMLConfigSHA256 {
			Log.info("[MDMManager] xml-config unchanged (SHA256: \(hash)), skipping")
			return
		}
		lastXMLConfigSHA256 = hash

		do {
			try config.loadFromXmlString(buffer: xmlConfig)
			Log.info("[MDMManager] xml-config applied (\(xmlConfig.count) chars)")
		} catch let error {
			Log.error("[MDMManager] Failed loading xml-config: error = \(error) xml = \(xmlConfig)")
		}
	}

	func applyMdmConfigToCore(core: Core) {
		guard !isApplyingConfig else { return }
		isApplyingConfig = true
		defer {
			isApplyingConfig = false
			_ = managedConfigChangedSinceLastCheck()
		}

		loadXMLConfigFromMdm(config: core.config!)

		guard let mdmConfig = UserDefaults.standard.dictionary(forKey: "com.apple.configuration.managed") else {
			if hasMDMConfig {
				Log.info("[MDMManager] Managed configuration was removed")
				lastXMLConfigSHA256 = nil
				lastCoreConfigSHA256 = nil
				hasMDMConfig = false
				handleConfigurationRemoved()
			} else {
				Log.info("[MDMManager] Managed configuration is empty")
			}
			return
		}

		hasMDMConfig = true

		let subset: [String: Any] = [
			"root-ca": mdmConfig["root-ca"] ?? "",
			"config-uri": mdmConfig["config-uri"] ?? ""
		]
		let hash = sha256Hash(of: subset)
		if hash == lastCoreConfigSHA256 {
			Log.info("[MDMManager] root-ca/config-uri unchanged (SHA256: \(hash)), skipping")
			NotificationCenter.default.post(name: MDMManager.configurationAppliedNotification, object: self, userInfo: ["config": mdmConfig])
			return
		}
		lastCoreConfigSHA256 = hash

		let currentProvisioningUri = core.provisioningUri

		if let rootCa = mdmConfig["root-ca"] as? String {
			core.rootCaData = rootCa
			Log.info("[MDMManager] root-ca applied (\(rootCa.count) chars)")
		}
		if let configUri = mdmConfig["config-uri"] as? String {
			do {
				if configUri != currentProvisioningUri {
					try core.setProvisioninguri(newValue: configUri)
					Log.info("[MDMManager] config-uri applied \(configUri)")
				}
			} catch let error {
				Log.error("[MDMManager] Failed setting provisioning URI: error = \(error) configUri = \(configUri)")
			}
		}
		
		if core.globalState == .On {
			do {
				core.stop()
				try core.start()
			} catch let error {
				Log.error("[MDMManager] Failed restarting core: error = \(error)")
			}
		}

		NotificationCenter.default.post(name: MDMManager.configurationAppliedNotification, object: self, userInfo: ["config": mdmConfig])
	}

	private func handleConfigurationRemoved() {
		Log.info("[MDMManager] handleConfigurationRemoved - posting notification")
		NotificationCenter.default.post(name: MDMManager.configurationRemovedNotification, object: self)
	}

	private func sha256Hash(of dict: [String: Any]) -> String {
		let description = dict.sorted(by: { $0.key < $1.key }).map { "\($0.key)=\($0.value)" }.joined(separator: "&")
		let digest = SHA256.hash(data: Data(description.utf8))
		return digest.map { String(format: "%02x", $0) }.joined()
	}
}
