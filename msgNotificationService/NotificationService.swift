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

var LINPHONE_DUMMY_SUBJECT = "dummy subject"

let appGroupName: String = {
    Bundle.main.object(forInfoDictionaryKey: "APP_GROUP_NAME") as? String
    ?? {
        fatalError("APP_GROUP_NAME not defined in Info.plist")
    }()
}()

extension String {
    func getDisplayNameFromSipAddress(lc: Core) -> String? {
        Log.info("looking for display name for \(self)")
        
        let defaults = UserDefaults.init(suiteName: appGroupName)
        let addressBook = defaults?.dictionary(forKey: "addressBook")
        
        if addressBook == nil {
            Log.info("address book not found in userDefaults")
            return nil
        }
        
        var usePrefix = true
        if let account = lc.defaultAccount, let params = account.params {
            usePrefix = params.useInternationalPrefixForCallsAndChats
        }
        
        if let simpleAddr = lc.interpretUrl(url: self, applyInternationalPrefix: usePrefix) {
            simpleAddr.clean()
            let nomalSipaddr = simpleAddr.asString()
            if let displayName = addressBook?[nomalSipaddr] as? String {
                Log.info("display name for \(self): \(displayName)")
                return displayName
            }
        }
        
        Log.info("display name for \(self) not found in userDefaults")
        return nil
    }
}

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
    var coreDelegate: CoreDelegate?
    var iterateTimer: DispatchSourceTimer?

    override init() {
        super.init()
#if USE_CRASHLYTICS
        FirebaseApp.configure()
#endif
    }
    
    override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
        let timeStart = Date.now
        self.contentHandler = contentHandler
        bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)
        
        LoggingService.Instance.logLevel = LogLevel.Debug
        Factory.Instance.logCollectionPath = Factory.Instance.getDataDir(context: UnsafeMutablePointer<Int8>(mutating: (appGroupName as NSString).utf8String))
        Factory.Instance.enableLogCollection(state: LogCollectionState.Enabled)
        
        Log.info("[msgNotificationService] start msgNotificationService extension")
        /*
         if (VFSUtil.vfsEnabled(groupName: AppServices.config.appGroupName) && !VFSUtil.activateVFS()) {
         VFSUtil.log("[VFS] Error unable to activate.", .error)
         }
         */
        
        if let bestAttemptContent = bestAttemptContent {
            if let aps = request.content.userInfo["aps"] as? [String: Any], let alert = aps["alert"] as? [String: Any], let locKey = alert["loc-key"] as? String, locKey == "MWI_NOTIFY_STR" {
                bestAttemptContent.title = String(localized: "Voicemail")
                bestAttemptContent.body = String(localized: "New message")
                contentHandler(bestAttemptContent)
                return
            }
            
			if !createCore() {
				bestAttemptContent.title = String(localized: "notification_chat_message_received_title")
				contentHandler(bestAttemptContent)
				return
			}
            if !lc!.config!.getBool(section: "app", key: "disable_chat_feature", defaultValue: false) {
                Log.info("received push payload : \(bestAttemptContent.userInfo.debugDescription)")
                
                if let defaultAccountParams = lc?.defaultAccount?.params, defaultAccountParams.publishEnabled == true {
                    let params = defaultAccountParams
                    let clonedParams = params.clone()
                    clonedParams?.publishEnabled = false
                    lc?.defaultAccount?.params = clonedParams
                }
                
                if let chatRoomInviteAddr = bestAttemptContent.userInfo["chat-room-addr"] as? String, !chatRoomInviteAddr.isEmpty {
                    bestAttemptContent.body = String(localized: "GC_MSG")
                    Log.info("fetch chat room for invite, addr: \(chatRoomInviteAddr)")
                    let chatRoom = lc?.getNewChatRoomFromConfAddr(chatRoomAddr: chatRoomInviteAddr)
                    Log.info("chat room invite received from: \(chatRoom?.subject ?? "unknown")")
                    stopCore()
                    contentHandler(UNNotificationContent())
                    return

                } else if let callId = bestAttemptContent.userInfo["call-id"] as? String {
                    Log.info("fetch msg for callid ["+callId+"]")
                    let message = lc!.getNewMessageFromCallid(callId: callId)
                    
                    if let message = message {
                        
                        let nilParams: ConferenceParams? = nil
                        if let peerAddr = message.peerAddr
                            , let chatroom = lc!.searchChatRoom(params: nilParams, localAddr: nil, remoteAddr: peerAddr, participants: nil), chatroom.muted {
                            Log.info("message comes from a muted chatroom, ignore it")
                            stopCore()
                            contentHandler(UNNotificationContent())
                            return
                        }
                        let msgData = parseMessage(message: message)
                        
                        // Extension only upates app's badge when main shared core is Off = extension's core is On.
                        // Otherwise, the app will update the badge.
                        if lc?.globalState == GlobalState.On, let badge = updateBadge() as NSNumber? {
                            bestAttemptContent.badge = badge
                        }
                        
                        bestAttemptContent.sound = UNNotificationSound(named: UNNotificationSoundName(rawValue: "msg.caf"))
                        bestAttemptContent.title = String(localized: "notification_chat_message_received_title")
                        if let subtitle = msgData?.subtitle {
                            bestAttemptContent.subtitle = subtitle
                        }
                        if let body = msgData?.body {
                            bestAttemptContent.body = body
                        }
                        
                        bestAttemptContent.categoryIdentifier = "msg_cat"
                        
                        bestAttemptContent.userInfo.updateValue(msgData?.callId as Any, forKey: "CallId")
                        bestAttemptContent.userInfo.updateValue(msgData?.from as Any, forKey: "from")
                        bestAttemptContent.userInfo.updateValue(msgData?.peerAddr as Any, forKey: "peer_addr")
                        bestAttemptContent.userInfo.updateValue(msgData?.localAddr as Any, forKey: "local_addr")
                        
                        // Do not display any notification if it was a reaction being removed
                        let content: UNNotificationContent =
                            message.reactionContent == " " ? UNNotificationContent() : bestAttemptContent

                        // start remaining time count 25 instead of 30 to make sure we keep at least 5 seconds to stop the core
                        stopCoreThenDisplay(content, notLaterThan: timeStart.addingTimeInterval(25))
                        return
                    } else {
                        Log.info("Message not found for callid ["+callId+"]")
                        stopCore()
                        contentHandler(UNNotificationContent())
                        return
                    }
                } else {
                    stopCore()
                    contentHandler(UNNotificationContent())
                    return
                }
            } else {
                stopCore()
                contentHandler(UNNotificationContent())
                return
            }
        }
    }
    
    override func serviceExtensionTimeWillExpire() {
        // Called just before the extension will be terminated by the system.
        // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
        Log.warn("serviceExtensionTimeWillExpire")
        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[msgNotificationService] serviceExtensionTimeWillExpire")
            bestAttemptContent.categoryIdentifier = "app_active"
            if let chatRoomInviteAddr = bestAttemptContent.userInfo["chat-room-addr"] as? String, !chatRoomInviteAddr.isEmpty {
                stopCore()
                contentHandler(UNNotificationContent())
                return
            } else if let callId = bestAttemptContent.userInfo["call-id"] as? String {
                stopCore()
                bestAttemptContent.title = String(localized: "notification_chat_message_received_title")
                bestAttemptContent.body = NSLocalizedString("IM_MSG", comment: "")
                
                contentHandler(bestAttemptContent)
                return
            } else {
                stopCore()
                contentHandler(UNNotificationContent())
                return
            }
        }
    }
    
    func parseMessage(message: PushNotificationMessage) -> MsgData? {
        
        var content = ""
        if message.isConferenceInvitationNew {
            content = String(localized: "message_meeting_invitation_notification")
        } else if message.isConferenceInvitationUpdate {
            content =  String(localized: "message_meeting_invitation_updated_notification")
        } else if message.isConferenceInvitationCancellation {
            content =  String(localized: "message_meeting_invitation_cancelled_notification")
        } else {
            content = message.isText ? message.textContent! : "🗻"
        }
        
        let fromAddr = message.fromAddr?.username
        let callId = message.callId
        let localUri = message.localAddr?.asStringUriOnly()
        let peerUri = message.peerAddr?.asStringUriOnly()
        let reactionContent = message.reactionContent
        let from: String
        if let fromDisplayName = message.fromAddr?.asStringUriOnly().getDisplayNameFromSipAddress(lc: lc!) {
            from = fromDisplayName
        } else {
            from = fromAddr!
        }
        
        var msgData = MsgData(from: fromAddr, body: "", subtitle: "", callId: callId, localAddr: localUri, peerAddr: peerUri)
        
        if let showMsg = lc!.config?.getBool(section: "ui", key: "display_notification_content", defaultValue: true), showMsg == true {
            msgData.subtitle = message.subject ?? from
            if reactionContent == nil {
                msgData.body = (message.subject != nil ? "\(from): " : "") + content
            } else {
                msgData.body = String(format: String(localized: "notification_chat_message_reaction_received"), from, reactionContent!, content)
            }
        } else {
            if let subject = message.subject {
                msgData.body = subject + ": " + from
            } else {
                msgData.body = from
            }
        }
        
        Log.info("received msg size : \(content.count) \n")
        return msgData
    }
    
	func createCore() -> Bool {
		Log.info("[msgNotificationService] create core")

		let factoryPath = FileUtil.bundleFilePath("linphonerc-factory")!
		if let config = Config.newForSharedCore(appGroupId: appGroupName, configFilename: "linphonerc", factoryConfigFilename: factoryPath) {
			lc = try? Factory.Instance.createSharedCoreWithConfig(config: config, systemContext: nil, appGroupId: appGroupName, mainCore: false)
			return lc != nil
		} else {
			return false
		}
	}
    
    func stopCore() {
        Log.info("stop core")
        if let lc = lc {
            lc.stop()
        }
    }

    // Stops the core and displays the notification as soon as it reaches Off, so that pending IMDNs are sent
    // first: the core stays in Shutdown while they are in flight. If running on flexisip 2.4 or older, will
    // wait for timeout configured by config variable 'misc' > 'delay_message_send_app_ext_s'.
    //
    // Waiting for Off can never be open ended. linphone_core_stop_async() is meant to force the shutdown after
    // misc/max_stop_async_time, but that safety net is armed by sal_begin_background_task(), which returns 0 in
    // an app extension: the core then stays in Shutdown for as long as the server keeps talking to it, which a
    // stream of conference NOTIFYs does indefinitely. So we bound the wait on the delay the IMDNs actually need
    // and force the stop past it. The core must always end up Off: that is what releases the shared core for
    // the next pushes.
    func stopCoreThenDisplay(_ content: UNNotificationContent, notLaterThan hardDeadline: Date) {
        guard let lc = lc, lc.globalState == GlobalState.On else {
            // Already stopped, typically by a main core starting up: nothing left to wait for.
            display(content)
            return
        }

        let imdnDelay = lc.config?.getInt(section: "misc", key: "delay_message_send_app_ext_s", defaultValue: 3) ?? 3
        let deadline = min(Date.now.addingTimeInterval(TimeInterval(imdnDelay) + 1), hardDeadline)

        coreDelegate = CoreDelegateStub(onGlobalStateChanged: { [weak self] (_: Core, gstate: GlobalState, _: String) in
            // Called from within linphone_core_iterate(), where stopping the core is forbidden. It is off anyway.
            if gstate == .Off {
                self?.display(content, coreIsOff: true)
            }
        })
        lc.addDelegate(delegate: coreDelegate!)
        lc.stopAsync()

        // Auto iterate does not work for app extension, so we manualy loop.
        // This is what sends the IMDNs and ends the shutdown
        let timer = DispatchSource.makeTimerSource(queue: .main)
        timer.schedule(deadline: .now(), repeating: .milliseconds(20))
        timer.setEventHandler { [weak self] in
            guard let self = self, self.contentHandler != nil else { return }
            self.lc?.iterate()
            if Date.now >= deadline {
                Log.warn("core did not stop by itself, forcing it")
                self.display(content)
            }
        }
        iterateTimer = timer
        timer.resume()
    }

    // Single exit point of the message path: stops the core, then displays. Only the first call counts.
    func display(_ content: UNNotificationContent, coreIsOff: Bool = false) {
        guard let contentHandler = contentHandler else { return }
        self.contentHandler = nil
        iterateTimer?.cancel()
        iterateTimer = nil
        if !coreIsOff {
            stopCore()
        }
        contentHandler(content)
    }

    func updateBadge() -> Int {
        var count = 0
        count += lc!.unreadChatMessageCount
        count += lc!.missedCallsCount
        count += lc!.callsNb
        Log.info("badge: \(count)\n")
        
        return count
    }
    
}

// swiftlint:enable identifier_name
