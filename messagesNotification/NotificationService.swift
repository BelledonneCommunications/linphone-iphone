//
//  NotificationService.swift
//  messagesNotification
//
//  Created by Paul Cartier on 13/09/2019.
//

import UserNotifications
import linphonesw

var running: Bool = true
var msgFrom: String?
var msgContent: String?
var callId: String?
var localUri: String?
var peerUri: String?

class NotificationService: UNNotificationServiceExtension {

    var contentHandler: ((UNNotificationContent) -> Void)?
    var bestAttemptContent: UNMutableNotificationContent?
    
    var lc: Core?
    let coreManager = LinphoneCoreManager()
    
    override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
        self.contentHandler = contentHandler
        bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)
        
        FileManager.exploreSharedContainer()
        
        if let bestAttemptContent = bestAttemptContent, let appActive = FileManager.getAppStatus() {
            
            NSLog("[EXTENSION] app active: \(appActive)")
            if (!appActive) {
                startCore()

                if let badge = updateBadge() as NSNumber? {
                    bestAttemptContent.badge = badge
                }

                lc!.networkReachable = false
                lc!.stop()
            

                bestAttemptContent.sound = UNNotificationSound(named: UNNotificationSoundName(rawValue: "msg.caf"))
                bestAttemptContent.title = "Message received [extension]"
                if let msgFrom = msgFrom {
                    bestAttemptContent.subtitle = msgFrom
                }
                if let msgContent = msgContent {
                    bestAttemptContent.body = msgContent
                }

                bestAttemptContent.categoryIdentifier = "msg_cat"
                
                bestAttemptContent.userInfo.updateValue(callId, forKey: "CallId")
                bestAttemptContent.userInfo.updateValue(msgFrom, forKey: "from")
                bestAttemptContent.userInfo.updateValue(peerUri, forKey: "peer_addr")
                bestAttemptContent.userInfo.updateValue(localUri, forKey: "local_addr")
            } else {
                bestAttemptContent.categoryIdentifier = "app_active"
            }

            contentHandler(bestAttemptContent)
        }
    }
    
    override func serviceExtensionTimeWillExpire() {
        // Called just before the extension will be terminated by the system.
        // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[EXTENSION] TIME OUT")
            lc!.stop()
            // Modify the notification content here...
            bestAttemptContent.title = "\(bestAttemptContent.title) [time out]"
            contentHandler(bestAttemptContent)
        }
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
            lc!.addDelegate(delegate: coreManager)
            try! lc!.start()
            NSLog("[EXTENSION] core started")
            lc!.refreshRegisters()
//            register()
//            NSLog("[EXTENSION] register launched")
            
            while(running) {
                lc!.iterate() /* first iterate initiates registration */
                usleep(50000)
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
}


class LinphoneCoreManager: CoreDelegate {
    
    override func onRegistrationStateChanged(lc: Core, cfg: ProxyConfig, cstate: RegistrationState, message: String?) {
        NSLog("[EXTENSION] New registration state \(cstate) for user id \( String(describing: cfg.identityAddress?.asString()))\n")
    }
    
    override func onMessageReceived(lc: Core, room: ChatRoom, message: ChatMessage) {
        NSLog("[EXTENSION] Core received msg \n")
        msgFrom = message.fromAddress?.username
        
        if (message.isText) {
            msgContent = message.textContent
        } else {
            msgContent = "🗻"
        }
        NSLog("[EXTENSION] msg: \(msgContent) \n")

        callId = message.getCustomHeader(headerName: "Call-Id")
//        content.userInfo = @{@"from" : from, @"peer_addr" : peer_uri, @"local_addr" : local_uri, @"CallId" : callID, @"msgs" : msgs};
        
        peerUri = room.peerAddress?.asStringUriOnly()
        localUri = room.localAddress?.asStringUriOnly()

        
        running = false
    }
    
    override func onNotifyReceived(lc: Core, lev: Event, notifiedEvent: String, body: Content) {
        NSLog("[EXTENSION] onNotifyReceived \n")
    }
    
    override func onMessageReceivedUnableDecrypt(lc: Core, room: ChatRoom, message: ChatMessage) {
        NSLog("[EXTENSION] onMessageReceivedUnableDecrypt \n")
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
        return FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: "group.org.linphone.phone.messagesNotification")!
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
        let defaults = UserDefaults(suiteName: "group.org.linphone.phone.messagesNotification")
        return defaults?.bool(forKey: "appActive")
    }
}
