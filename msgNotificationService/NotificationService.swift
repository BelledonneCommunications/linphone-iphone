//
//  NotificationService.swift
//  messagesNotification
//
//  Created by Paul Cartier on 13/09/2019.
//

import UserNotifications
import linphonesw

var GROUP_ID = "group.org.linphone.phone.msgNotification"

struct SenderData: Codable {
    var msgFrom: String?
    var msgContent: String?
    var callId: String?
    var localUri: String?
    var peerUri: String?
}

var senderData: SenderData?
var running: Bool = true

class NotificationService: UNNotificationServiceExtension {

    var contentHandler: ((UNNotificationContent) -> Void)?
    var bestAttemptContent: UNMutableNotificationContent?
    
    var lc: Core?
    
    override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
        self.contentHandler = contentHandler
        bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)
        
        
        
//        if let bestAttemptContent = bestAttemptContent {
//            // Modify the notification content here...
//            //            bestAttemptContent.title = "\(bestAttemptContent.title) [modified]"
//            bestAttemptContent.categoryIdentifier = "paul"
//            bestAttemptContent.title = "Nouveau message"
//            bestAttemptContent.body = "You have received a message."
//            contentHandler(bestAttemptContent)
//        }
        
        
        registerForAppNotifications()
        FileManager.exploreSharedContainer()

        if let bestAttemptContent = bestAttemptContent, let appActive = FileManager.getAppStatus() {

            NSLog("[EXTENSION] app active: \(appActive)")
            if (!appActive) {
                FileManager.setExtensionStatus(extActive: true);
                startCore()

                if let badge = updateBadge() as NSNumber? {
                    bestAttemptContent.badge = badge
                }

                lc!.networkReachable = false
                lc!.stop()

                bestAttemptContent.sound = UNNotificationSound(named: UNNotificationSoundName(rawValue: "msg.caf"))
                bestAttemptContent.title = "Message received [extension]"
                if let msgFrom = senderData?.msgFrom {
                    bestAttemptContent.subtitle = msgFrom
                }
                if let msgContent = senderData?.msgContent {
                    bestAttemptContent.body = msgContent
                }

                bestAttemptContent.categoryIdentifier = "paul"

                bestAttemptContent.userInfo.updateValue(senderData?.callId, forKey: "CallId")
                bestAttemptContent.userInfo.updateValue(senderData?.msgFrom, forKey: "from")
                bestAttemptContent.userInfo.updateValue(senderData?.peerUri, forKey: "peer_addr")
                bestAttemptContent.userInfo.updateValue(senderData?.localUri, forKey: "local_addr")



            } else {
                bestAttemptContent.categoryIdentifier = "app_active"
                bestAttemptContent.title = "Message received [time out]"
                bestAttemptContent.body = "You have received a message."
            }

            FileManager.setExtensionStatus(extActive: false);
            contentHandler(bestAttemptContent)
        }
    }
    
    override func serviceExtensionTimeWillExpire() {
        // Called just before the extension will be terminated by the system.
        // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
//        if let lc = lc {
//            lc.stop()
//        }
//        FileManager.setExtensionStatus(extActive: false);
        
        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[EXTENSION] TIME OUT")
            bestAttemptContent.categoryIdentifier = "app_active"
            bestAttemptContent.title = "Message received [time out]"
            bestAttemptContent.body = "You have received a message."
            contentHandler(bestAttemptContent)
        }
    }

    func appNotificationCallback() {
        NSLog("[DARWIN] notif recue")
        if let lc = lc {
            lc.stop()
        }
        FileManager.setExtensionStatus(extActive: false);

        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[EXTENSION] STOPPED BY APP")
            bestAttemptContent.categoryIdentifier = "app_active"
            bestAttemptContent.title = "\(bestAttemptContent.title) [killed]"
//            sleep(4)
            contentHandler(bestAttemptContent)
        }
    }

    func registerForAppNotifications() {
        let notification = CFNotificationCenterGetDarwinNotifyCenter()
        let notifName = "STOP_EXT" as CFString
        let observer = UnsafeRawPointer(Unmanaged.passUnretained(self).toOpaque())
        
        CFNotificationCenterAddObserver(notification, observer, { (_, observer, _, _, _) -> Void in
            if let observer = observer {
                let mySelf = Unmanaged<NotificationService>.fromOpaque(observer).takeUnretainedValue()
                mySelf.appNotificationCallback()
            }
        }, notifName, nil, .deliverImmediately)
    }
    
    func startCore() {
        running = true
        
        let log = LoggingService.Instance /*enable liblinphone logs.*/
        let logManager = LinphoneLoggingServiceManager()
        log.logLevel = LogLevel.Message
        log.addDelegate(delegate: logManager)
        
        do {
//            Instanciate a LinphoneCore object
            lc = try Factory.Instance.createCore(configPath: FileManager.preferenceFile(file: "linphonerc").path, factoryConfigPath: "", systemContext: nil)
            
//            NSLog("[EXTENSION] core successfully created")
            let coreManager = LinphoneCoreManager()
            lc!.addDelegate(delegate: coreManager)
            
            try! lc!.start()
            NSLog("[EXTENSION] core started")
            lc!.refreshRegisters()
//            register()
//            NSLog("[EXTENSION] register launched")
            var i = 0
            while(running) {
                lc!.iterate() /* first iterate initiates registration */
                NSLog("[EXTENSION] \(i)")
                i += 1;
                usleep(100000)
            }
            
        } catch {
            NSLog("[EXTENSION] \(error)")
        }
    }
    
    func updateBadge() -> Int {
        var count = 0
        count += lc!.unreadChatMessageCount
        count += lc!.missedCallsCount
        count += lc!.callsNb
        NSLog("[EXTENSION] badge: \(count)\n")
        
        return count
    }
    
    
    class LinphoneCoreManager: CoreDelegate {
        
        override func onRegistrationStateChanged(lc: Core, cfg: ProxyConfig, cstate: RegistrationState, message: String?) {
            NSLog("[EXTENSION] New registration state \(cstate) for user id \( String(describing: cfg.identityAddress?.asString()))\n")
        }
        
        override func onMessageReceived(lc: Core, room: ChatRoom, message: ChatMessage) {
            NSLog("[EXTENSION] Core received msg \n")
            // content.userInfo = @{@"from" : from, @"peer_addr" : peer_uri, @"local_addr" : local_uri, @"CallId" : callID, @"msgs" : msgs};
            
            //        LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event_log);
            //        if (!chat)
            //        return;
            
            if (message.contentType == "application/im-iscomposing+xml") {
                return
            }
            
            //        var msgContent: String
            //
            //        if (message.isText) {
            //            msgContent = message.textContent
            //        } else if (message.isFileTransfer) {
            //            msgContent = "🗻"
            //        } else {
            //            return
            //        }
            let msgContent = message.isText ? message.textContent : "🗻"
            let msgFrom = message.fromAddress?.username
            let callId = message.getCustomHeader(headerName: "Call-Id")
            let localUri = room.localAddress?.asStringUriOnly()
            let peerUri = room.peerAddress?.asStringUriOnly()
            
            senderData = SenderData(msgFrom: msgFrom, msgContent: msgContent, callId: callId, localUri: localUri, peerUri: peerUri)
            NSLog("[EXTENSION] msg: \(senderData?.msgContent) \n")
            
            running = false // TODO PAUL : mettre au début de la function
        }
        
        override func onNotifyReceived(lc: Core, lev: Event, notifiedEvent: String, body: Content) {
            NSLog("[EXTENSION] onNotifyReceived \n")
        }
        
        override func onMessageReceivedUnableDecrypt(lc: Core, room: ChatRoom, message: ChatMessage) {
            NSLog("[EXTENSION] onMessageReceivedUnableDecrypt \n")
        }
    }
}




class LinphoneLoggingServiceManager: LoggingServiceDelegate {
    override func onLogMessageWritten(logService: LoggingService, domain: String, lev: LogLevel, message: String) {
        let level: String
        
        switch lev {
        case .Debug:
            level = "Debug"
        case .Trace:
            level = "Trace"
        case .Message:
            level = "Message"
        case .Warning:
            level = "Warning"
        case .Error:
            level = "Error"
        case .Fatal:
            level = "Fatal"
        default:
            level = "unknown"
        }
    
        NSLog("[EXTENSION] \(level) Logging service log: \(message)\n")
    }
}

extension FileManager {
    static func sharedContainerURL() -> URL {
        return FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: GROUP_ID)!
    }
    
    static func exploreSharedContainer() {
        if let content = try? FileManager.default.contentsOfDirectory(atPath: FileManager.sharedContainerURL().path) {
            content.forEach { file in
                NSLog(file)
            }
        }
    }
    
    static func preferenceFile(file: String) -> URL {
        let fullPath = FileManager.sharedContainerURL().appendingPathComponent("Library/Preferences/linphone/")
        return fullPath.appendingPathComponent(file)
    }
    
    static func dataFile(file: String) -> URL {
        let fullPath = FileManager.sharedContainerURL().appendingPathComponent("Library/Application Support/linphone/")
        return fullPath.appendingPathComponent(file)
    }
    
    static func getAppStatus() -> Bool? {
        let defaults = UserDefaults(suiteName: GROUP_ID)
        return defaults?.bool(forKey: "appActive")
    }
    
    static func setExtensionStatus(extActive: Bool) {
        let defaults = UserDefaults(suiteName: GROUP_ID)
        defaults?.set(extActive, forKey: "extActive")
    }
}
