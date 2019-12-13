//
//  NotificationViewController.swift
//  msgNotificationContent
//
//  Created by Paul Cartier on 10/12/2019.
//

import UIKit
import UserNotifications
import UserNotificationsUI
import linphonesw


var GROUP_ID = "group.org.linphone.phone.msgNotification"
var isRegistered: Bool = false
var isReplySent: Bool = false
var isImdnDelivered: Bool = false

class NotificationViewController: UIViewController, UNNotificationContentExtension {

    @IBOutlet var label: UILabel?
    var lc: Core?

//    override func viewWillDisappear(_ animated: Bool) {
//        super.viewWillDisappear(animated)
//        lc?.stop() // TODO PAUL : garder ca si il y a un call pour supprimer le core?
//    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any required interface initialization here.
        
        
        
        
//        // Define the custom actions.
//        let acceptAction = UNNotificationAction(identifier: "ACCEPT_ACTION",
//                                                title: "Accept",
//                                                options: UNNotificationActionOptions(rawValue: 0))
//        let declineAction = UNNotificationAction(identifier: "DECLINE_ACTION",
//                                                 title: "Decline",
//                                                 options: UNNotificationActionOptions(rawValue: 0))
//        // Define the notification type
//        let meetingInviteCategory =
//            UNNotificationCategory(identifier: "MEETING_INVITATION",
//                                   actions: [acceptAction, declineAction],
//                                   intentIdentifiers: [],
//                                   hiddenPreviewsBodyPlaceholder: "",
//                                   options: .customDismissAction)
//        // Register the notification type.
//        let notificationCenter = UNUserNotificationCenter.current()
//        notificationCenter.setNotificationCategories([meetingInviteCategory])
        
        
        
        
//        let likeAction = UNNotificationAction(identifier: "like", title: "Like", options: [])
//        let saveAction = UNNotificationAction(identifier: "save", title: "Save", options: [])
//        let category = UNNotificationCategory(identifier: "paul", actions: [likeAction, saveAction], intentIdentifiers: [], options: [])
//        UNUserNotificationCenter.current().setNotificationCategories([category])
        
        
//        let replyAction = UNNotificationAction(identifier: "Reply", title: "Reply", options: [])
        
        let replyAction = UNTextInputNotificationAction(identifier: "Reply",
                         title: "Reply",
                         options: [],
                         textInputButtonTitle: "Send",
                         textInputPlaceholder: "")
        
        let seenAction = UNNotificationAction(identifier: "Seen", title: "Mark as seen", options: [])
        let category = UNNotificationCategory(identifier: "paul", actions: [replyAction, seenAction], intentIdentifiers: [], options: [.customDismissAction])
        UNUserNotificationCenter.current().setNotificationCategories([category])

//        UNNotificationCategory *cat_msg =
//            [UNNotificationCategory categoryWithIdentifier:@"msg_cat"
//                actions:[NSArray arrayWithObjects:act_reply, act_seen, nil]
//                intentIdentifiers:[[NSMutableArray alloc] init]
//                options:UNNotificationCategoryOptionCustomDismissAction];
        
        // TODO PAUL handle dismiss action
    }
    
    func didReceive(_ notification: UNNotification) {
//        self.label?.text = notification.request.content.body // title
        self.label?.text = "test test test"
    }
    
    func didReceive(_ response: UNNotificationResponse,
                    completionHandler completion: @escaping (UNNotificationContentExtensionResponseOption) -> Void) {
        let userInfo = response.notification.request.content.userInfo
        switch response.actionIdentifier {
        case "Reply":
            if let replyText = response as? UNTextInputNotificationResponse {
                completion(.dismiss)
                replyAction(userInfo, text: replyText.userText)
            }
            break
        case "Seen":
            completion(.dismiss)
            markAsSeenAction(userInfo)
            break
        default:
            break
        }
        
//        completion(.dismiss) // TODO PAUL : ok mais dans le cas on ouvre la notif : .dissmisAndForward -> open app conv -> deja fait?
//        var i = 0
//        while(i < 50) {
//            NSLog("[EXTENSION] test test \(i)")
//            i += 1;
//            usleep(100000)
//        }
    }
    
    func markAsSeenAction(_ userInfo: [AnyHashable : Any]) {
        NSLog("[EXTENSION] markAsSeenAction")
        
        startCore()
        let peerAddress = userInfo["peer_addr"] as! String
        let localAddress = userInfo["local_addr"] as! String
        let peer = try! lc!.createAddress(address: peerAddress)
        let local = try! lc!.createAddress(address: localAddress)
        let room = lc!.findChatRoom(peerAddr: peer, localAddr: local)
        if let room = room {
//            let roomDelegate = LinphoneChatRoomManager()
//            room.addDelegate(delegate: roomDelegate)
            room.markAsRead()
        }
        
//        var i = 0
//        while(!isImdnDelivered) {
            lc!.iterate()
//            NSLog("[EXTENSION] \(i)")
//            i += 1;
//            usleep(100000)
//        }
        lc!.stop()
    }
    
    func replyAction(_ userInfo: [AnyHashable : Any], text replyText: String) {
        NSLog("[EXTENSION] replyAction")
        
        //    NSString *replyText = [(UNTextInputNotificationResponse *)response userText];
//        let replyText = "réponse"
        
//        UnsafeMutablePointer(cObject), "swiftRef",  UnsafeMutableRawPointer(Unmanaged.passUnretained(sObject).toOpaque())
        
        
        startCore()
        let peerAddress = userInfo["peer_addr"] as! String
        let localAddress = userInfo["local_addr"] as! String
        let peer = try! lc!.createAddress(address: peerAddress)
        let local = try! lc!.createAddress(address: localAddress)
        let room = lc!.findChatRoom(peerAddr: peer, localAddr: local)
        if let room = room {
            let msgDelegate = LinphoneChatMessageManager()
            let chatMsg = try! room.createMessage(message: replyText)
            chatMsg.addDelegate(delegate: msgDelegate)
            room.sendChatMessage(msg: chatMsg)
            room.markAsRead()
        }
        
        var i = 0
        while(!isReplySent) {
            lc!.iterate()
            NSLog("[EXTENSION] \(i)")
            i += 1;
            usleep(100000)
        }
        lc!.stop()
        
        
//    } else if ([response.actionIdentifier isEqual:@"Reply"]) {
//    NSString *replyText = [(UNTextInputNotificationResponse *)response userText];
//    NSString *peer_address = [response.notification.request.content.userInfo objectForKey:@"peer_addr"];
//    NSString *local_address = [response.notification.request.content.userInfo objectForKey:@"local_addr"];
//    LinphoneAddress *peer = linphone_address_new(peer_address.UTF8String);
//    LinphoneAddress *local = linphone_address_new(local_address.UTF8String);
//    LinphoneChatRoom *room = linphone_core_find_chat_room(LC, peer, local);
//    if(room)
//    [LinphoneManager.instance send:replyText toChatRoom:room];
//
//    linphone_address_unref(peer);
//    linphone_address_unref(local);
    }
    
    
    
    func startCore() {
        let log = LoggingService.Instance /*enable liblinphone logs.*/
        let logManager = LinphoneLoggingServiceManager()
        log.logLevel = LogLevel.Message
        log.addDelegate(delegate: logManager)
        
        do {
            lc = try Factory.Instance.createCore(configPath: FileManager.preferenceFile(file: "linphonerc").path, factoryConfigPath: "", systemContext: nil)
            lc = try Factory.Instance.createSharedCore(configPath: <#T##String#>, factoryConfigFilename: <#T##String#>, systemContext: <#T##UnsafeMutableRawPointer?#>, appGroup: <#T##String#>, mainCore: <#T##Bool#>)
            // NSLog("[EXTENSION] core successfully created")
            let coreManager = LinphoneCoreManager()
            lc!.addDelegate(delegate: coreManager)
            
            try! lc!.start()
            // NSLog("[EXTENSION] core started")
            lc!.refreshRegisters()
            
            var i = 0
            while(!isRegistered) {
                lc!.iterate()
                NSLog("[EXTENSION] \(i)")
                i += 1;
                usleep(100000)
            }
        } catch {
            NSLog("[EXTENSION] \(error)")
        }
    }
    
    
    
    class LinphoneCoreManager: CoreDelegate {
        override func onRegistrationStateChanged(lc: Core, cfg: ProxyConfig, cstate: RegistrationState, message: String?) {
            NSLog("[EXTENSION] New registration state \(cstate) for user id \( String(describing: cfg.identityAddress?.asString()))\n")
            if (cstate == .Ok) {
                isRegistered = true
            }
        }
    }
    
//    class LinphoneChatRoomManager: ChatRoomDelegate {
//        override func onImdnDelivered(cr: ChatRoom) {
//            NSLog("[EXTENSION] onImdnDelivered \n")
//            isImdnDelivered = true
//        }
//    }

    class LinphoneChatMessageManager: ChatMessageDelegate {
        override func onMsgStateChanged(msg: ChatMessage, state: ChatMessage.State) {
            NSLog("[EXTENSION] onMsgStateChanged: \(state)\n")
            if (state == .InProgress) {
                isReplySent = true
            }
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
