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

import SwiftUI
import linphonesw
import UserNotifications

let accountTokenNotification = Notification.Name("AccountCreationTokenReceived")
var displayedChatroomPeerAddr: String?

class AppDelegate: NSObject, UIApplicationDelegate, UNUserNotificationCenterDelegate {
	
	var launchNotificationCallId: String?
	var launchNotificationPeerAddr: String?
	var launchNotificationLocalAddr: String?
	
	var navigationManager: NavigationManager?
	
	func application(_ application: UIApplication, didRegisterForRemoteNotificationsWithDeviceToken deviceToken: Data) {
		let tokenStr = deviceToken.map { String(format: "%02.2hhx", $0) }.joined()
		Log.info("Received remote push token : \(tokenStr)")
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("Forwarding remote push token to core")
			core.didRegisterForRemotePushWithStringifiedToken(deviceTokenStr: tokenStr + ":remote")
		}
	}
	
	func application(_ application: UIApplication, didFailToRegisterForRemoteNotificationsWithError error: Error) {
		Log.error("Failed to register for push notifications : \(error.localizedDescription)")
	}
	
	func application(_ application: UIApplication, didReceiveRemoteNotification userInfo: [AnyHashable: Any], fetchCompletionHandler completionHandler: @escaping (UIBackgroundFetchResult) -> Void) {
		Log.info("Received background push notification, payload = \(userInfo.description)")
		let creationToken = (userInfo["customPayload"] as? NSDictionary)?["token"] as? String
		if let creationToken = creationToken {
			NotificationCenter.default.post(name: accountTokenNotification, object: nil, userInfo: ["token": creationToken])
		}
		
		completionHandler(UIBackgroundFetchResult.newData)
	}
					 
	func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
		// Set up notifications
		UNUserNotificationCenter.current().delegate = self
		
		return true
	}
	
	// Called when the user interacts with the notification
	func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
		let userInfo = response.notification.request.content.userInfo
		
		if let callId = userInfo["CallId"] as? String, let peerAddr = userInfo["peer_addr"] as? String, let localAddr = userInfo["local_addr"] as? String {
			if self.navigationManager != nil {
				self.navigationManager!.selectedCallId = callId
				self.navigationManager!.peerAddr = peerAddr
				self.navigationManager!.localAddr = localAddr
			} else {
				launchNotificationCallId = callId
				launchNotificationPeerAddr = peerAddr
				launchNotificationLocalAddr = localAddr
			}
		}
		
		completionHandler()
	}
	
	// Display notifications on foreground
	func userNotificationCenter(_ center: UNUserNotificationCenter, willPresent notification: UNNotification, withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
		let userInfo = notification.request.content.userInfo
		Log.info("Received push notification in foreground, payload= \(userInfo)")
		
		let strPeerAddr = userInfo["peer_addr"] as? String
		if strPeerAddr == nil {
			completionHandler([.banner, .sound])
		} else {
			// Only display notification if we're not in the chatroom they come from
			if displayedChatroomPeerAddr != strPeerAddr {
				CoreContext.shared.doOnCoreQueue { core in
					let nilParams: ConferenceParams? = nil
					if 	let peerAddr = try? Factory.Instance.createAddress(addr: strPeerAddr!)
							, let chatroom = core.searchChatRoom(params: nilParams, localAddr: nil, remoteAddr: peerAddr, participants: nil), chatroom.muted {
						Log.info("message comes from a muted chatroom, ignore it")
						return
					}
					completionHandler([.banner, .sound])
				}
			}
		}
			
	}
	
	func applicationWillTerminate(_ application: UIApplication) {
		Log.info("IOS applicationWillTerminate")
		CoreContext.shared.doOnCoreQueue(synchronous: true) { core in
			Log.info("applicationWillTerminate - Stopping linphone core")
			MagicSearchSingleton.shared.destroyMagicSearch()
			if core.globalState != GlobalState.Off {
				core.stop()
			} else {
				Log.info("applicationWillTerminate - Core already stopped")
			}
		}
	}
}

@main
struct LinphoneApp: App {
	
	@Environment(\.scenePhase) var scenePhase
	@UIApplicationDelegateAdaptor(AppDelegate.self) var delegate
	@StateObject var navigationManager = NavigationManager()
	
	@ObservedObject private var coreContext = CoreContext.shared
	
	@State var index: Int = 0
	
	/*
	@State private var editContactViewModel: EditContactViewModel?
	@State private var historyViewModel: HistoryViewModel?
	@State private var historyListViewModel: HistoryListViewModel?
	@State private var startCallViewModel: StartCallViewModel?
	@State private var startConversationViewModel: StartConversationViewModel?
	@State private var callViewModel: CallViewModel?
	@State private var meetingWaitingRoomViewModel: MeetingWaitingRoomViewModel?
	@State private var conversationsListViewModel: ConversationsListViewModel?
	@State private var conversationViewModel: ConversationViewModel?
	@State private var meetingsListViewModel: MeetingsListViewModel?
	@State private var meetingViewModel: MeetingViewModel?
	@State private var conversationForwardMessageViewModel: ConversationForwardMessageViewModel?
	@State private var accountProfileViewModel: AccountProfileViewModel?
	*/
	
	@State private var pendingURL: URL?
	
	var body: some Scene {
		WindowGroup {
			if coreContext.coreHasStartedOnce {
				ZStack {
					if !SharedMainViewModel.shared.welcomeViewDisplayed {
						ZStack {
							WelcomeView()
							
							ToastView()
								.zIndex(3)
						}
					} else if (coreContext.coreIsStarted && coreContext.accounts.isEmpty) || SharedMainViewModel.shared.displayProfileMode {
						ZStack {
							AssistantView()
							
							ToastView()
								.zIndex(3)
						}
					} else {
						ContentView(
							//editContactViewModel: editContactViewModel!,
							//historyViewModel: historyViewModel!,
							//historyListViewModel: historyListViewModel!,
							//startCallViewModel: startCallViewModel!,
							//startConversationViewModel: startConversationViewModel!,
							//callViewModel: callViewModel!,
							//meetingWaitingRoomViewModel: meetingWaitingRoomViewModel!,
							//conversationsListViewModel: conversationsListViewModel!,
							//conversationViewModel: conversationViewModel!,
							//meetingsListViewModel: meetingsListViewModel!,
							//meetingViewModel: meetingViewModel!,
							//conversationForwardMessageViewModel: conversationForwardMessageViewModel!,
							//accountProfileViewModel: accountProfileViewModel!,
							index: $index
						)
						.environmentObject(navigationManager)
						.onAppear {
							index = SharedMainViewModel.shared.indexView
							// Link the navigation manager to the AppDelegate
							delegate.navigationManager = navigationManager
							
							// Check if the app was launched with a notification payload
							if let callId = delegate.launchNotificationCallId, let peerAddr = delegate.launchNotificationPeerAddr, let localAddr = delegate.launchNotificationLocalAddr {
								// Notify the app to navigate to the chat room
								navigationManager.openChatRoom(callId: callId, peerAddr: peerAddr, localAddr: localAddr)
							}
							
							//accountProfileViewModel!.setAvatarModel()
						}
					}
					
					if coreContext.coreIsStarted {
						VStack {
							
						}
						.onAppear {
							if let url = pendingURL {
								URIHandler.handleURL(url: url)
								pendingURL = nil
							}
						}
					}
				}
				.onOpenURL { url in
					if coreContext.coreIsStarted {
						URIHandler.handleURL(url: url)
					} else {
						pendingURL = url
					}
				}
			} else {
				SplashScreen()
			}
		}.onChange(of: scenePhase) { newPhase in
			if !TelecomManager.shared.callInProgress {
				if newPhase == .active {
					Log.info("Entering foreground")
					coreContext.onEnterForeground()
				} else if newPhase == .inactive {
				} else if newPhase == .background {
					Log.info("Entering background")
					coreContext.onEnterBackground()
				}
			}
		}
	}
}
