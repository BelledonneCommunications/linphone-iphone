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
import linphone

let APP_GROUP_ID = "group.org.linphone.phone.msgNotification"

extension String: Error {}

class LocalPushProvider: NEAppPushProvider {
	var core: Core? = nil
	let log = LoggingService.Instance
	var logDelegate: LinphoneLoggingServiceManager!
	var coreDelegateStub : CoreDelegateStub? = nil
	let defaults = UserDefaults.init(suiteName: APP_GROUP_ID)
	var coreIteratorTimer:Timer? = nil
	var aggretatorTimer:Timer? = nil
	var aggregagor:[ChatMessage] = []
	
	func chatRoomMuted(chatRoom:ChatRoom) -> Bool {
		if let chatroomsPushStatus = defaults?.dictionary(forKey: "chatroomsPushStatus"),  let from = chatRoom.peerAddress?.asStringUriOnly()  {
				if ((chatroomsPushStatus[from] as? String) == "disabled") {
					return true
				}
		}
		return false
	}
	
	func createCore() throws {
		coreDelegateStub = CoreDelegateStub(
			onMessageReceived: { (core:Core, chatRoom:ChatRoom, message:ChatMessage) -> Void in
				if (self.ignoredContentTypes.contains(message.contentType)) {
					self.log.error(message: "Received unexpected content type.\(message.contentType)")
				} else if (!self.chatRoomMuted(chatRoom: chatRoom)) {
					self.aggregagor.append(message)
				}
			}
		)
		guard let configString = providerConfiguration?["coreconfig"] as? String, let config = Config.newFromBuffer(buffer: configString)  else {
			log.error(message: "Unable to get core config through provider configuration")
			throw "Unable to get core config through provider configuration"
		}
		logDelegate = try LinphoneLoggingServiceManager(config: config, log: log, domain: "LocalPushProvider")
		
		// Ensure a separate UUID from app is used, use previously generated one or a new one if n/a.
		
		if let uuid = defaults?.string(forKey: "misc_uuid") {
			config.setString(section: "misc", key: "uuid", value: uuid)
		} else {
			config.cleanEntry(section: "misc", key: "uuid")
		}
		
		log.message(message: "Creating LocalPushProvider core with configuration : \(config.dump())")
		core = try Factory.Instance.createCoreWithConfig(config: config, systemContext: nil)
		core?.autoIterateEnabled = false // 20ms auto-iterations are too frequent for NE, sometimes it gets killed by the OS. (triggers limit exceed of 150 wakeups per second over 300 seconds)
		core?.addDelegate(delegate: coreDelegateStub!)
		coreIteratorTimer = Timer(timeInterval: 0.1, target: self, selector: #selector(iterate), userInfo: nil, repeats: true) // 0.1 second
		RunLoop.main.add(coreIteratorTimer!, forMode: .default)
		
		let aggregateTime = config.getInt(section: "local_push", key: "notif_aggregation_period", defaultValue: 3)
		aggretatorTimer = Timer(timeInterval: TimeInterval(aggregateTime), target: self, selector: #selector(flushAggregator), userInfo: nil, repeats: true) // 0.1 second
		RunLoop.main.add(aggretatorTimer!, forMode: .default)

		
		core?.accountList.forEach { account in
			let params = account.params?.clone()
			params?.expires = 60 // handleTimerEvent(), called by the OS, refreshes registers every 60 seconds, we don't need to expiration longer as if it's not refreshed it means the extension is not reachable.
			account.params = params
		}
	}
	
	@objc func iterate() {
		core?.iterate()
	}
	
	@objc func flushAggregator() {
		if (aggregagor.count == 1) {
			showLocalNotification(message: aggregagor[0])
		} else if (aggregagor.count > 1) {
			var displayNames : [String] = []
			aggregagor.forEach { message in
				let displayName = getDisplayName(message: message)
				if (!displayNames.contains(displayName)) {
					displayNames.append(displayName)
				}
			}
			displayNames.sort()
			showMultipleMessagesNotifications(count: aggregagor.count,displayNames: displayNames.joined(separator: ","))
		}
		aggregagor.removeAll()
	}
	

	
	// MARK: - NEAppPushProvider Life Cycle
		
	override func start(completionHandler: @escaping (Error?) -> Void) {
		do {
			if (core == nil) {
				try createCore()
				log.message(message: "Creating core")
			}
			try core?.start()
			coreIteratorTimer?.fire()
			aggretatorTimer?.fire()
			log.message(message: "Core started")
			// Keep freshly generated UUID after start to avoid re-creating one every time the NE is starting.
			if (core?.config?.hasEntry(section: "misc", key: "uuid") != 0) {
				defaults?.set(core?.config?.getString(section: "misc", key: "uuid", defaultString: ""), forKey: "misc_uuid")
				log.message(message: "storing generated UUID \(String(describing: defaults?.string(forKey: "misc_uuid")))")
			}
			completionHandler(nil)
		} catch {
			completionHandler(error)
		}
	}
	
	override func stop(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
		log.message(message: "Received stop for reason \(reason)")
		core?.stopAsync()
		coreIteratorTimer?.invalidate()
		aggretatorTimer?.invalidate()
		flushAggregator()
		completionHandler()
	}
	
	override func handleTimerEvent() {
		log.message(message: "Refreshing registers (handleTimerEvent)")
		core?.refreshRegisters()
	}
	
	// MARK: - Notify User
	
	let ignoredContentTypes = ["message/imdn+xml","application/im-iscomposing+xml"]
	
	func showMultipleMessagesNotifications(count:Int, displayNames: String) {
		let content = UNMutableNotificationContent()
		content.title = NSLocalizedString("%s messages received", comment: "").replacingOccurrences(of: "%s", with: String(count))
		content.body =  NSLocalizedString("from: %s", comment: "").replacingOccurrences(of: "%s", with: displayNames)
		content.sound = UNNotificationSound(named: UNNotificationSoundName("msg.caf"))
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
	
	func getDisplayName(message:ChatMessage) -> String {
		let fromAddr = message.chatRoom?.peerAddress?.asStringUriOnly()
		var displayName = fromAddr?.getDisplayNameFromSipAddress(lc: core!, logger: log, groupId: APP_GROUP_ID)
		displayName = displayName != nil ? displayName : message.chatRoom?.peerAddress?.displayName
		displayName = displayName != nil && displayName?.isEmpty != true ? displayName :message.chatRoom?.peerAddress?.username
		return displayName!
	}
	
	func showLocalNotification(message: ChatMessage) {
		
		var messageContent = ""
		if (message.hasConferenceInvitationContent()) {
			messageContent = NSLocalizedString("📅 You are invited to a meeting", comment: "")
		} else {
			messageContent = message.hasTextContent() ? message.utf8Text ?? "" : "🗻"
		}
		
		let content = UNMutableNotificationContent()
		content.title = getDisplayName(message:message)
		content.body = messageContent
		content.sound = UNNotificationSound(named: UNNotificationSoundName("msg.caf"))
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
