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

// swiftlint:disable identifier_name

import UserNotifications
import linphonesw
#if USE_CRASHLYTICS
import Firebase
#endif

var APP_GROUP_ID = "group.org.linphone.phone.msgNotification"
var LINPHONE_DUMMY_SUBJECT = "dummy subject"

struct MsgData: Codable {
    var from: String?
    var body: String?
    var subtitle: String?
    var callId: String?
    var localAddr: String?
    var peerAddr: String?
}

class NotificationService: UNNotificationServiceExtension {

    var contentHandler: ((UNNotificationContent) -> Void)?
    var bestAttemptContent: UNMutableNotificationContent?

    var lc: Core?
//    static var logDelegate: LinphoneLoggingServiceManager!
//	static var log: LoggingService!
	
	override init() {
		super.init()
#if USE_CRASHLYTICS
		FirebaseApp.configure()
#endif
	}

    override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {

    }

    override func serviceExtensionTimeWillExpire() {
        // Called just before the extension will be terminated by the system.
        // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
		//NotificationService.log.warning(message: "serviceExtensionTimeWillExpire")
		//stopCore()
        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[msgNotificationService] serviceExtensionTimeWillExpire")
            bestAttemptContent.categoryIdentifier = "app_active"

			if let chatRoomInviteAddr = bestAttemptContent.userInfo["chat-room-addr"] as? String, !chatRoomInviteAddr.isEmpty {
				bestAttemptContent.title = NSLocalizedString("GC_MSG", comment: "")
				bestAttemptContent.body = ""
				bestAttemptContent.sound = UNNotificationSound(named: UNNotificationSoundName("msg.caf")) // TODO : temporary fix, to be removed after flexisip release
			} else {
				bestAttemptContent.title = NSLocalizedString("Message received", comment: "")
				bestAttemptContent.body = NSLocalizedString("IM_MSG", comment: "")
			}
            contentHandler(bestAttemptContent)
        }
    }

}

// swiftlint:enable identifier_name
