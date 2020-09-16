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

import UIKit
import UserNotifications
import UserNotificationsUI
import linphonesw
#if USE_CRASHLYTICS
import Firebase
#endif

var APP_GROUP_ID = "group.org.linphone.phone.msgNotification"
var isReplySent: Bool = false
var needToStop: Bool = false
var coreStopped: Bool = false
var log: LoggingService!

class NotificationViewController: UIViewController, UNNotificationContentExtension, ChatMessageDelegate, CoreDelegate {

    var lc: Core?
    var config: Config!
    var logDelegate: LinphoneLoggingServiceManager!

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any required interface initialization here.
#if USE_CRASHLYTICS
		FirebaseApp.configure()
#endif
        NSLog("[msgNotificationContent] start msgNotificationContent extension")

        let replyAction = UNTextInputNotificationAction(identifier: "Reply",
                         title: NSLocalizedString("Reply", comment: ""),
                         options: [],
                         textInputButtonTitle: NSLocalizedString("Send", comment: ""),
                         textInputPlaceholder: "")

        let seenAction = UNNotificationAction(identifier: "Seen", title: NSLocalizedString("Mark as seen", comment: ""), options: [])
        let category = UNNotificationCategory(identifier: "msg_cat", actions: [replyAction, seenAction], intentIdentifiers: [], options: [.customDismissAction])
        UNUserNotificationCenter.current().setNotificationCategories([category])

        needToStop = false
        isReplySent = false
        coreStopped = false
    }

    func didReceive(_ notification: UNNotification) {}

    func didReceive(_ response: UNNotificationResponse,
                    completionHandler completion: @escaping (UNNotificationContentExtensionResponseOption) -> Void) {
        do {
            let userInfo = response.notification.request.content.userInfo
            switch response.actionIdentifier {
            case "Reply":
                if let replyText = response as? UNTextInputNotificationResponse {
                    try replyAction(userInfo, text: replyText.userText)
                }
                break
            case "Seen":
                try markAsSeenAction(userInfo)
                break
            default:
                break
            }

            if (needToStop) {
                log.error(message: "core stopped by app")
                throw LinphoneError.timeout
            } else {
                completion(.dismiss)
                stopCore()
            }

        } catch {
            log.error(message: "error: \(error)")
            completion(.dismissAndForwardAction)
        }
    }

    func markAsSeenAction(_ userInfo: [AnyHashable : Any]) throws {
        NSLog("[msgNotificationContent] markAsSeenAction")
        try startCore()

        let peerAddress = userInfo["peer_addr"] as! String
        let localAddress = userInfo["local_addr"] as! String
        let peer = try lc!.createAddress(address: peerAddress)
        let local = try lc!.createAddress(address: localAddress)
        let room = lc!.findChatRoom(peerAddr: peer, localAddr: local)
        if let room = room {
            room.markAsRead()
        }
        lc!.iterate()
    }

    func replyAction(_ userInfo: [AnyHashable : Any], text replyText: String) throws {
        NSLog("[msgNotificationContent] replyAction")
        try startCore()

        let peerAddress = userInfo["peer_addr"] as! String
        let localAddress = userInfo["local_addr"] as! String
        let peer = try lc!.createAddress(address: peerAddress)
        let local = try lc!.createAddress(address: localAddress)
        let room = lc!.findChatRoom(peerAddr: peer, localAddr: local)
        if let room = room {
            let chatMsg = try room.createMessage(message: replyText)
            chatMsg.addDelegate(delegate: self)
			chatMsg.send()
            room.markAsRead()
        }

        for i in 0...50 where !isReplySent && !needToStop {
            log.debug(message: "reply \(i)")
            lc!.iterate()
            usleep(10000)
        }
    }

    func startCore() throws {
		config = Config.newForSharedCore(appGroupId: APP_GROUP_ID, configFilename: "linphonerc", factoryConfigFilename: "")
		log = LoggingService.Instance /*enable liblinphone logs.*/
		logDelegate = try! LinphoneLoggingServiceManager(config: config, log: log, domain: "msgNotificationContent")
        lc = try! Factory.Instance.createSharedCoreWithConfig(config: config, systemContext: nil, appGroupId: APP_GROUP_ID, mainCore: false)

        lc!.addDelegate(delegate: self)

        try lc!.start()
        log.message(message: "core started")

        if (needToStop) {
            log.error(message: "core stopped by app")
            throw LinphoneError.timeout
        }
    }

    func stopCore() {
        lc!.stopAsync()
        log.message(message: "stop core")
        for i in 0...100 where !coreStopped {
            log.debug(message: "stop \(i)")
            lc!.iterate()
            usleep(50000)
        }
    }

	func onGlobalStateChanged(core: Core, state gstate: GlobalState, message: String) {
		log.message(message: "global state changed: \(gstate) : \(message) \n")
		if (gstate == .Shutdown) {
			needToStop = true
		} else if (gstate == .Off) {
			coreStopped = true
		}
	}

	func onMsgStateChanged(message: ChatMessage, state: ChatMessage.State) {
		log.message(message: "msg state changed: \(state)\n")
		if (state == .Delivered) {
			isReplySent = true
		}
	}
}
