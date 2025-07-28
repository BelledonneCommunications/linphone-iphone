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

extension String {
	func getDisplayNameFromSipAddress(lc: Core) -> String? {
		Log.info("looking for display name for \(self)")
		
		let defaults = UserDefaults.init(suiteName: Config.appGroupName)
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
	
	override init() {
		super.init()
#if USE_CRASHLYTICS
		FirebaseApp.configure()
#endif
	}
	
	override func didReceive(_ request: UNNotificationRequest, withContentHandler contentHandler: @escaping (UNNotificationContent) -> Void) {
		
		self.contentHandler = contentHandler
		bestAttemptContent = (request.content.mutableCopy() as? UNMutableNotificationContent)
		
		LoggingService.Instance.logLevel = LogLevel.Debug
		Factory.Instance.logCollectionPath = Factory.Instance.getConfigDir(context: nil)
		Factory.Instance.enableLogCollection(state: LogCollectionState.Enabled)
		Log.info("[msgNotificationService] start msgNotificationService extension")
		/*
		if (VFSUtil.vfsEnabled(groupName: Config.appGroupName) && !VFSUtil.activateVFS()) {
			VFSUtil.log("[VFS] Error unable to activate.", .error)
		}
		*/
        
		if let bestAttemptContent = bestAttemptContent {
			createCore()
			if !lc!.config!.getBool(section: "app", key: "disable_chat_feature", defaultValue: false) {
				Log.info("received push payload : \(bestAttemptContent.userInfo.debugDescription)")
			
			if let defaultAccountParams = lc?.defaultAccount?.params, defaultAccountParams.publishEnabled == true {
				let params = defaultAccountParams
				let clonedParams = params.clone()
				clonedParams?.publishEnabled = false
				lc?.defaultAccount?.params = clonedParams
			}
				
				/*
				let defaults = UserDefaults.init(suiteName: Config.appGroupName)
				if let chatroomsPushStatus = defaults?.dictionary(forKey: "chatroomsPushStatus") {
					let aps = bestAttemptContent.userInfo["aps"] as? NSDictionary
					let alert = aps?["alert"] as? NSDictionary
					let fromAddresses = alert?["loc-args"] as? [String]
					
					if let from = fromAddresses?.first {
						if ((chatroomsPushStatus[from] as? String) == "disabled") {
							NotificationService.log.message(message: "message comes from a muted chatroom, ignore it")
							contentHandler(UNNotificationContent())
						}
					}
				}
				*/
				if let chatRoomInviteAddr = bestAttemptContent.userInfo["chat-room-addr"] as? String, !chatRoomInviteAddr.isEmpty {
					Log.info("fetch chat room for invite, addr: \(chatRoomInviteAddr), ignore it")
					if let chatRoom = lc?.getNewChatRoomFromConfAddr(chatRoomAddr: chatRoomInviteAddr) {
						Log.info("chat room invite received from: \(chatRoom.subject ?? "unknown")")
						/*
						bestAttemptContent.title = NSLocalizedString("GC_MSG", comment: "")
						if chatRoom.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) {
							if chatRoom.peerAddress != nil {
								if chatRoom.peerAddress!.displayName != nil && chatRoom.peerAddress!.displayName!.isEmpty != true {
									bestAttemptContent.body = chatRoom.peerAddress!.displayName!
								} else if chatRoom.peerAddress!.username != nil {
									bestAttemptContent.body = chatRoom.peerAddress!.username!
								} else {
									bestAttemptContent.body = String(chatRoom.peerAddress!.asStringUriOnly().dropFirst(4))
								}
							} else {
								bestAttemptContent.body = "Peer Address Error"
							}
						} else {
							bestAttemptContent.body = chatRoom.subject!
						}
						contentHandler(bestAttemptContent)
						return
						*/
					}
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
						
						stopCore()
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
						if message.reactionContent != " " {
							contentHandler(bestAttemptContent)
						} else {
							contentHandler(UNNotificationContent())
						}
						
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
				/*
				bestAttemptContent.title = NSLocalizedString("GC_MSG", comment: "")
				bestAttemptContent.body = ""
				bestAttemptContent.sound = UNNotificationSound(named: UNNotificationSoundName("msg.caf"))
				*/
				let _ = lc?.getNewChatRoomFromConfAddr(chatRoomAddr: chatRoomInviteAddr)
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
		
		if let showMsg = lc!.config?.getBool(section: "app", key: "show_msg_in_notif", defaultValue: true), showMsg == true {
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
	
	func createCore() {
		Log.info("[msgNotificationService] create core")

		lc = try? Factory.Instance.createSharedCoreWithConfig(config: Config.get(), systemContext: nil, appGroupId: Config.appGroupName, mainCore: false)
	}
	
	func stopCore() {
		Log.info("stop core")
		if let lc = lc {
			lc.stop()
		}
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
