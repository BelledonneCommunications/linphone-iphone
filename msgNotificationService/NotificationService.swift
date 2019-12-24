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

    enum LinphoneCoreError: Error {
        case timeout
    }

    var contentHandler: ((UNNotificationContent) -> Void)?
    var bestAttemptContent: UNMutableNotificationContent?

    var lc: Core?
    var config: Config!
    var logDelegate: LinphoneLoggingServiceManager!
    var coreDelegate: LinphoneCoreManager!

    override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
        self.contentHandler = contentHandler
        bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)

        if let bestAttemptContent = bestAttemptContent {
            do {
                try startCore()

                if let badge = updateBadge() as NSNumber? {
                    bestAttemptContent.badge = badge
                }

                stopCore()

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
                NSLog("[EXTENSION] failed to start shared core")
                serviceExtensionTimeWillExpire()
            }
        }
    }

    override func serviceExtensionTimeWillExpire() {
        // Called just before the extension will be terminated by the system.
        // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push payload will be used.
        stopCore()
        if let contentHandler = contentHandler, let bestAttemptContent =  bestAttemptContent {
            NSLog("[EXTENSION] TIME OUT")
            bestAttemptContent.categoryIdentifier = "app_active"
            bestAttemptContent.title = NSLocalizedString("Message received", comment: "") + " [time out]" // TODO PAUL : a enlever
            bestAttemptContent.body = NSLocalizedString("You have received a message.", comment: "")          
            contentHandler(bestAttemptContent)
        }
    }

    func startCore() throws {
        msgReceived = false

        config = Config.newWithFactory(configFilename: FileManager.preferenceFile(file: "linphonerc").path, factoryConfigFilename: "")
        setCoreLogger(config: config)
        lc = try! Factory.Instance.createSharedCoreWithConfig(config: config, systemContext: nil, appGroup: GROUP_ID, mainCore: false)

        coreDelegate = LinphoneCoreManager(self)
        lc!.addDelegate(delegate: coreDelegate)

        try lc!.start()

        NSLog("[EXTENSION] core started")
        lc!.refreshRegisters()

        for i in 0...100 where !msgReceived {
            lc!.iterate()
            NSLog("[EXTENSION] \(i)")
            usleep(100000)
        }

        if (!msgReceived) {
            throw LinphoneCoreError.timeout
        }
    }

    func stopCore() {
        if let lc = lc {
            if let coreDelegate = coreDelegate {
                lc.removeDelegate(delegate: coreDelegate)
            }
            lc.networkReachable = false
            lc.stop()
        }
    }

    func setCoreLogger(config: Config) {
        let debugLevel = config.getInt(section: "app", key: "debugenable_preference", defaultValue: LogLevel.Debug.rawValue)
        let debugEnabled = (debugLevel >= LogLevel.Debug.rawValue && debugLevel < LogLevel.Error.rawValue)

        if (debugEnabled) {
            let log = LoggingService.Instance /*enable liblinphone logs.*/
            logDelegate = LinphoneLoggingServiceManager()
            log.logLevel = LogLevel(rawValue: debugLevel)
            log.addDelegate(delegate: logDelegate)
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
