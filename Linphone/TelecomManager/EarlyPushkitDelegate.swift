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

import PushKit
import CallKit
import UserNotifications

class EarlyPushkitDelegate: NSObject, PKPushRegistryDelegate, CXProviderDelegate {
	private var activeCalls: [UUID: CXProvider] = [:]

	func providerDidReset(_ provider: CXProvider) {}

	func provider(_ provider: CXProvider, perform action: CXAnswerCallAction) {
		Log.info("[EarlyPushkitDelegate] User tried to answer, ending call as device is locked")
		action.fail()
		provider.reportCall(with: action.callUUID, endedAt: .init(), reason: .unanswered)
		activeCalls.removeValue(forKey: action.callUUID)
		postMissedCallNotification()
	}

	func pushRegistry(_ registry: PKPushRegistry, didUpdate pushCredentials: PKPushCredentials, for type: PKPushType) {
		Log.info("[EarlyPushkitDelegate] Received push credentials, ignoring until core is ready")
	}

	func pushRegistry(_ registry: PKPushRegistry, didReceiveIncomingPushWith payload: PKPushPayload, for type: PKPushType, completion: @escaping () -> Void) {
		Log.info("[EarlyPushkitDelegate] Received incoming push while core is not ready, reporting call to CallKit")
		let providerConfig = CXProviderConfiguration()
		providerConfig.supportsVideo = false
		let provider = CXProvider(configuration: providerConfig)
		provider.setDelegate(self, queue: .main)

		let update = CXCallUpdate()
		update.remoteHandle = CXHandle(type: .generic, value: NSLocalizedString("early_push_unknown_caller", comment: ""))
		update.hasVideo = false
		let uuid = UUID()
		activeCalls[uuid] = provider

		provider.reportNewIncomingCall(with: uuid, update: update) { error in
			if let error = error {
				Log.error("[EarlyPushkitDelegate] Failed to report call to CallKit: \(error.localizedDescription)")
			}
			completion()
		}

		DispatchQueue.main.asyncAfter(deadline: .now() + 4) { [weak self] in
			guard let self = self, let provider = self.activeCalls.removeValue(forKey: uuid) else { return }
			Log.info("[EarlyPushkitDelegate] Ending unanswered call after timeout")
			provider.reportCall(with: uuid, endedAt: .init(), reason: .unanswered)
			self.postMissedCallNotification()
		}
	}

	private func postMissedCallNotification() {
		let content = UNMutableNotificationContent()
		content.title = NSLocalizedString("early_push_missed_call_title", comment: "")
		content.body = NSLocalizedString("early_push_missed_call_body", comment: "")
		content.sound = .default
		let request = UNNotificationRequest(identifier: "early_push_missed_call", content: content, trigger: nil)
		UNUserNotificationCenter.current().add(request) { error in
			if let error = error {
				Log.error("[EarlyPushkitDelegate] Failed to post missed call notification: \(error.localizedDescription)")
			}
		}
	}
}
