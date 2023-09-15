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

import Foundation
import Combine
import NetworkExtension
import UserNotifications
import linphonesw
import os

let APP_GROUP_ID = "group.org.linphone.phone.msgNotification"

class LocalPushProvider: NEAppPushProvider, CoreDelegate {
	var core: Core? = nil
	let log = LoggingService.Instance
	var logDelegate: LinphoneLoggingServiceManager!
	var coreDelegateStub : CoreDelegateStub? = nil
	let defaults = UserDefaults.init(suiteName: APP_GROUP_ID)
	
	func createAndStartCore() {
		guard let configString = providerConfiguration?["coreconfig"] as? String, let config = Config.newFromBuffer(buffer: configString)  else {
			self.log.error(message: "Unable to get core config through provider configuration")
			return
		}
		
		// Ensure a separate UUID from app is used, use previously generated one or a new one if n/a.
		if let uuid = defaults?.string(forKey: "misc_uuid") {
			config.setString(section: "misc", key: "uuid", value: uuid)
		} else {
			config.cleanEntry(section: "misc", key: "uuid")
		}
		logDelegate = try! LinphoneLoggingServiceManager(config: config, log: log, domain: "LocalPushProvider")
		core = try! Factory.Instance.createCoreWithConfig(config: config, systemContext: nil)
		core?.autoIterateEnabled = false // 20ms auto-iterations are too frequent for NE, it gets killed by the OS. (limit of 150 wakeups per second over 300 seconds)
		core!.addDelegate(delegate: self)
		let timer = Timer(timeInterval: 0.5, target: self, selector: #selector(iterate), userInfo: nil, repeats: true) // 0.5 second
		RunLoop.main.add(timer, forMode: .default)
		try?core?.start()
		core?.addDelegate(delegate: coreDelegateStub!)
		log.message(message: "core started")
		
		// Keep generated UUID to avoid re-creating one every time the NE is starting.
		if (core!.config!.hasEntry(section: "misc", key: "uuid") != 0) {
			defaults?.set(core!.config!.getString(section: "misc", key: "uuid", defaultString: ""), forKey: "misc_uuid")
			self.log.message(message: "storing generated UUID \(String(describing: defaults?.string(forKey: "misc_uuid")))")
		}
	}
	
	@objc func iterate() {
		core?.iterate()
	}
	
	override init() {
		super.init()
		coreDelegateStub = CoreDelegateStub(
			onMessageReceived: { (core:Core, chatRoom:ChatRoom, message:ChatMessage) -> Void in
				self.showLocalNotification(message: message)
			}
		)
	}
	
	// MARK: - NEAppPushProvider Life Cycle
	
	override func start() {
		createAndStartCore()
	}
	
	override func stop(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
		core?.stopAsync()
		completionHandler()
	}
	
	override func handleTimerEvent() {
		self.log.message(message: "Refreshing registers (handleTimerEvent)")
		core?.refreshRegisters()
	}
	
	// MARK: - Notify User
	
	let ignoredContentTypes = ["message/imdn+xml","application/im-iscomposing+xml"]
	
	func showLocalNotification(message: ChatMessage) {
		
		if (ignoredContentTypes.contains(message.contentType)) {
			self.log.error(message: "Received unexpected content type.\(message.contentType)")
			return
		}
		
		var messageContent = ""
		if (message.hasConferenceInvitationContent()) {
			messageContent = NSLocalizedString("📅 You are invited to a meeting", comment: "")
		} else {
			messageContent = message.hasTextContent() ? message.utf8Text : "🗻"
		}
		
		let fromAddr = message.chatRoom?.peerAddress?.asStringUriOnly()
		var displayName = fromAddr?.getDisplayNameFromSipAddress(lc: core!, logger: log, groupId: APP_GROUP_ID)
		displayName = displayName != nil ? displayName : message.chatRoom?.peerAddress?.displayName
		displayName = displayName != nil && displayName?.isEmpty != true ? displayName :message.chatRoom?.peerAddress?.username
		
		let content = UNMutableNotificationContent()
		content.title = displayName!
		content.body = messageContent
		content.sound = .default
		content.categoryIdentifier = "app_active"
		let request = UNNotificationRequest(identifier: UUID().uuidString, content: content, trigger: nil)
		UNUserNotificationCenter.current().add(request) { error in
			if let error = error {
				self.log.error(message: "Error submitting local notification: \(error)")
				return
			}
			self.log.message(message: "Local notification posted successfully")
		}
	}
	
}
