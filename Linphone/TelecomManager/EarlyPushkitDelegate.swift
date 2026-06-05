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
	private var activeCalls: [String: (uuid: UUID, provider: CXProvider)] = [:]

	func providerDidReset(_ provider: CXProvider) {}

	func provider(_ provider: CXProvider, perform action: CXAnswerCallAction) {
		Log.info("[EarlyPushkitDelegate] User tried to answer, ending call as device is locked")
		action.fail()
		provider.reportCall(with: action.callUUID, endedAt: .init(), reason: .unanswered)
		activeCalls = activeCalls.filter { $0.value.uuid != action.callUUID }
		postMissedCallNotification(trigger: nil)
	}

	func pushRegistry(_ registry: PKPushRegistry, didUpdate pushCredentials: PKPushCredentials, for type: PKPushType) {
		Log.info("[EarlyPushkitDelegate] Received push credentials, ignoring until core is ready")
	}

	func pushRegistry(_ registry: PKPushRegistry, didReceiveIncomingPushWith payload: PKPushPayload, for type: PKPushType, completion: @escaping () -> Void) {
		Log.info("[EarlyPushkitDelegate] Received incoming push while core is not ready, reporting call to CallKit")
		let signature = String(describing: payload.dictionaryPayload as NSDictionary)

		if let existing = activeCalls[signature] {
			existing.provider.reportCall(with: existing.uuid, updated: makeCallUpdate())
			completion()
			return
		}

		let providerConfig = CXProviderConfiguration()
		providerConfig.supportsVideo = false
		let provider = CXProvider(configuration: providerConfig)
		provider.setDelegate(self, queue: .main)

		let uuid = UUID()
		activeCalls[signature] = (uuid, provider)

		provider.reportNewIncomingCall(with: uuid, update: makeCallUpdate()) { error in
			if let error = error {
				Log.error("[EarlyPushkitDelegate] Failed to report call to CallKit: \(error.localizedDescription)")
			}
		}

		postMissedCallNotification(trigger: UNTimeIntervalNotificationTrigger(timeInterval: 4, repeats: false))
		completion()

		DispatchQueue.main.asyncAfter(deadline: .now() + 4) { [weak self] in
			guard let self = self, let existing = self.activeCalls[signature], existing.uuid == uuid else { return }
			Log.info("[EarlyPushkitDelegate] Ending unanswered call after timeout")
			existing.provider.reportCall(with: uuid, endedAt: .init(), reason: .unanswered)
			self.activeCalls.removeValue(forKey: signature)
		}
	}

	private func makeCallUpdate() -> CXCallUpdate {
		let update = CXCallUpdate()
		update.remoteHandle = CXHandle(type: .generic, value: NSLocalizedString("early_push_unknown_caller", comment: ""))
		update.hasVideo = false
		return update
	}

	private func postMissedCallNotification(trigger: UNNotificationTrigger?) {
		let content = UNMutableNotificationContent()
		content.title = NSLocalizedString("early_push_missed_call_title", comment: "")
		content.body = NSLocalizedString("early_push_missed_call_body", comment: "")
		content.sound = .default
		content.interruptionLevel = .timeSensitive
		let request = UNNotificationRequest(identifier: "early_push_missed_call", content: content, trigger: trigger)
		UNUserNotificationCenter.current().add(request) { error in
			if let error = error {
				Log.error("[EarlyPushkitDelegate] Failed to post missed call notification: \(error.localizedDescription)")
			}
		}
	}
}
