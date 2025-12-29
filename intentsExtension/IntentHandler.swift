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

import Intents

class IntentHandler: INExtension {
	override func handler(for intent: INIntent) -> Any {
		return self
	}
}

// MARK: - Generic Call
extension IntentHandler: INStartCallIntentHandling {
	func handle(intent: INStartCallIntent,
				completion: @escaping (INStartCallIntentResponse) -> Void) {

		guard let number = intent.contacts?.first?.personHandle?.value else {
			completion(.init(code: .failure, userActivity: nil))
			return
		}

		let isVideo = intent.callCapability == .videoCall
		let activity = NSUserActivity(activityType: "org.linphone.startCall")
		activity.userInfo = ["number": number, "isVideo": isVideo]

		completion(.init(code: .continueInApp, userActivity: activity))
	}
}
