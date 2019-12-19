//
//  NotificationService.swift
//  messagesNotification
//
//  Created by Paul Cartier on 13/09/2019.
//

import UserNotifications
import linphonesw

var GROUP_ID = "group.org.linphone.phone.msgNotification"

struct MsgData: Codable {
    var from: String?
    var body: String?
    var subtitle: String?
    var callId: String?
    var localAddr: String?
    var peerAddr: String?
}

var msgData: MsgData?
var msgReceived: Bool = false

class NotificationService: UNNotificationServiceExtension {

    var contentHandler: ((UNNotificationContent) -> Void)?
    var bestAttemptContent: UNMutableNotificationContent?
    
    var lc: Core?
    
    override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
        self.contentHandler = contentHandler
        bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)
       
        
//        FileManager.exploreSharedContainer()

        if let bestAttemptContent = bestAttemptContent {
            do {
                try startCore()

                if let badge = updateBadge() as NSNumber? {
                    bestAttemptContent.badge = badge
                }

                lc!.networkReachable = false
                lc!.stop()

                bestAttemptContent.sound = UNNotificationSound(named: UNNotificationSoundName(rawValue: "msg.caf"))
                bestAttemptContent.title = NSLocalizedString("Message received", comment: "") + " [extension]"
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

                contentHandler(bestAttemptContent)
            } catch {
                NSLog("[EXTENSION] can't start shared core, another is started")
                serviceExtensionTimeWillExpire()
            }
        }
    }
    
    override func serviceExtensionTimeWillExpire() {
        // Called just before the extension will be terminated by the system.
        // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
        msgReceived = true;
        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[EXTENSION] TIME OUT")
            bestAttemptContent.categoryIdentifier = "app_active"
            bestAttemptContent.title = NSLocalizedString("Message received", comment: "") + " [time out]"
            bestAttemptContent.body = NSLocalizedString("You have received a message.", comment: "")
            lc?.stop()
            contentHandler(bestAttemptContent)
        }
    }
    
    func startCore() throws {
        msgReceived = false
        
        let log = LoggingService.Instance /*enable liblinphone logs.*/
        let logManager = LinphoneLoggingServiceManager()
        log.logLevel = LogLevel.Message
        log.addDelegate(delegate: logManager)
        
        lc = try! Factory.Instance.createSharedCore(configPath: FileManager.preferenceFile(file: "linphonerc").path, factoryConfigPath: "", systemContext: nil, appGroup: GROUP_ID, mainCore: false)
    
        let coreManager = LinphoneCoreManager(self)
        lc!.addDelegate(delegate: coreManager)
        
        try lc!.start()

        
        NSLog("[EXTENSION] core started")
        lc!.refreshRegisters()

        var i = 0
        while(!msgReceived) {
            lc!.iterate()
            NSLog("[EXTENSION] \(i)")
            i += 1;
            usleep(100000)
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
        unowned let parent: NotificationService
        
        init(_ parent: NotificationService) {
            self.parent = parent
        }
        
        override func onGlobalStateChanged(lc: Core, gstate: GlobalState, message: String) {
            NSLog("[EXTENSION] onGlobalStateChanged \(gstate) : \(message) \n")
            if (gstate == .Shutdown) {
                parent.serviceExtensionTimeWillExpire()
            }
        }
        
        override func onRegistrationStateChanged(lc: Core, cfg: ProxyConfig, cstate: RegistrationState, message: String?) {
            NSLog("[EXTENSION] New registration state \(cstate) for user id \( String(describing: cfg.identityAddress?.asString()))\n")
        }
        
        override func onMessageReceived(lc: Core, room: ChatRoom, message: ChatMessage) {
            NSLog("[EXTENSION] Core received msg \(message.contentType) \n")
            // content.userInfo = @{@"from" : from, @"peer_addr" : peer_uri, @"local_addr" : local_uri, @"CallId" : callID, @"msgs" : msgs};

            if (message.contentType != "text/plain" && message.contentType != "image/jpeg") {
                return
            }

            let content = message.isText ? message.textContent : "🗻"
            let from = message.fromAddress?.username
            let callId = message.getCustomHeader(headerName: "Call-Id")
            let localUri = room.localAddress?.asStringUriOnly()
            let peerUri = room.peerAddress?.asStringUriOnly()
            
            msgData = MsgData(from: from, body: "", subtitle: "", callId:callId, localAddr: localUri, peerAddr:peerUri)

            if let showMsg = lc.config?.getBool(section: "app", key: "show_msg_in_notif", defaultValue: true), showMsg == true {
                if let subject = room.subject as String?, subject != "" {
                    msgData?.subtitle = subject
                    msgData?.body = from! + " : " + content
                } else {
                    msgData?.subtitle = from
                    msgData?.body = content
                }
            } else {
                if let subject = room.subject as String?, subject != "" {
                    msgData?.body = subject + " : " + from!
                } else {
                    msgData?.body = from
                }
            }

            NSLog("[EXTENSION] msg: \(content) \n")
            msgReceived = true
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
    
        NSLog("[SDK] \(level): \(message)\n")
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
}
