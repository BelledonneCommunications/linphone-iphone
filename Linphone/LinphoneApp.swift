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

let accountTokenNotification = Notification.Name("AccountCreationTokenReceived")

class AppDelegate: NSObject, UIApplicationDelegate {
	
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
		/*
		let creationToken = (userInfo["customPayload"] as? NSDictionary)?["token"] as? String
		if let creationToken = creationToken {
			NotificationCenter.default.post(name: accountTokenNotification, object: nil, userInfo: ["token": creationToken])
		}
		completionHandler(UIBackgroundFetchResult.newData)*/
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
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@State private var contactViewModel: ContactViewModel?
	@State private var editContactViewModel: EditContactViewModel?
	@State private var historyViewModel: HistoryViewModel?
	@State private var historyListViewModel: HistoryListViewModel?
	@State private var startCallViewModel: StartCallViewModel?
	@State private var callViewModel: CallViewModel?
	@State private var meetingWaitingRoomViewModel: MeetingWaitingRoomViewModel?
	@State private var conversationsListViewModel: ConversationsListViewModel?
	@State private var conversationViewModel: ConversationViewModel?
	
	var body: some Scene {
		WindowGroup {
			if coreContext.coreIsStarted {
				if !sharedMainViewModel.welcomeViewDisplayed {
					WelcomeView()
				} else if coreContext.defaultAccount == nil || sharedMainViewModel.displayProfileMode {
					ZStack {
						AssistantView()
						
						ToastView()
							.zIndex(3)
					}
				} else if coreContext.defaultAccount != nil
							&& coreContext.loggedIn
							&& contactViewModel != nil
							&& editContactViewModel != nil
							&& historyViewModel != nil
							&& historyListViewModel != nil
							&& startCallViewModel != nil
							&& callViewModel != nil
							&& meetingWaitingRoomViewModel != nil
							&& conversationsListViewModel != nil
							&& conversationViewModel != nil {
					ContentView(
						contactViewModel: contactViewModel!,
						editContactViewModel: editContactViewModel!,
						historyViewModel: historyViewModel!,
						historyListViewModel: historyListViewModel!,
						startCallViewModel: startCallViewModel!,
						callViewModel: callViewModel!,
						meetingWaitingRoomViewModel: meetingWaitingRoomViewModel!,
						conversationsListViewModel: conversationsListViewModel!,
						conversationViewModel: conversationViewModel!
					)
				} else {
					SplashScreen()
				}
			} else {
				SplashScreen()
					.onDisappear {
						contactViewModel = ContactViewModel()
						editContactViewModel = EditContactViewModel()
						historyViewModel = HistoryViewModel()
						historyListViewModel = HistoryListViewModel()
						startCallViewModel = StartCallViewModel()
						callViewModel = CallViewModel()
						meetingWaitingRoomViewModel = MeetingWaitingRoomViewModel()
						conversationsListViewModel = ConversationsListViewModel()
						conversationViewModel = ConversationViewModel()
					}
			}
		}.onChange(of: scenePhase) { newPhase in
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
